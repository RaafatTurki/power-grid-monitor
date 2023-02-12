//power_grid_station.ino
///////////////////////////////////////////// Libs /////////////////////////////////////////////
#include <SPI.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <LiquidCrystal_I2C.h>

#include "ZMPT101B.h"

#include <SoftwareSerial.h>
#include <NMEAGPS.h>

#include <SX127XLT.h>

///////////////////////////////////////////// Preproceser Flags /////////////////////////////////////////////
#define LOG_FLAG_INFO 1
#define LOG_FLAG_WARN 2
#define LOG_FLAG_ERR 4
#define LOG_FLAG_TIMING 8
#define LOG_FLAG_COMS 16

///////////////////////////////////////////// Hardware Setup /////////////////////////////////////////////
//Pins
const byte GPS_TX_PIN = A0;                           //Does NOT need to be analog.
const byte GPS_RX_PIN = A1;                           //Does NOT need to be analog.

const byte SSR_PIN = 2;

const byte TEMP_SENSOR_PIN = A2;                      //Must be analog.
const byte VOLTAGE_SENSOR_PIN = A6;                   //Must be analog.
const byte CURRENT_SENSOR_PIN = A7;                   //Must be analog.

const byte LORA_NSS_PIN = 9;                          //Connected through pin 1 of the level converter.
//The rest of the LoRa pins must be connected to hardware SPI.
//MOSI -> Converter pin 2 -> Arduino pin 11
//MOSI -> Converter pin 3 -> Arduino pin 12
//SCK  -> Converter pin 4 -> Arduino pin 13
//Important: The LoRa is a 3.3V module!

//Display must be connected to hardware I2C.
//SDA -> A4
//SCL -> A5

//GPS
const unsigned int GPS_BAUDRATE = 9600;

//Voltage Sensor
const float VOLTAGE_SENSOR_SENSITIVITY = 0.00176;

//Current Sensor
const float CURRENT_SENSOR_SENSITIVITY = 0.02;
const float CURRENT_SENSOR_GND_OFFSET = 0.07;

//LoRa
#define LORA_DEVICE DEVICE_SX1278

//Display
const int DISPLAY_WIDTH = 16;
const int DISPLAY_HEIGHT = 2;
const byte DISPLAY_ADDRESS = 0x27;

//Network
const int NETWORK_ID = 1;


//Main Board
const float BOARD_VCC = 5.0;
const float BOARD_ADC_RESOLUTION = 1023.0;

///////////////////////////////////////////// Config /////////////////////////////////////////////
//General
#define LOG_LEVEL 31     //(LOG_TIMING + LOG_COMS)
//#define RUN_PERFORMANCE_ANALYTICS true

const unsigned int SERIAL_BAUDRATE = 9600;
const unsigned int TICK_RATE = 6;                            //Reads and transmits per second.

//Display
const float BOOT_UP_SCREEN_DURATION = 1000;                   //In milliseconds.

//GPS
const long GPS_REFRESH_FIXED = 60000;                         //In milliseconds.
const long GPS_REFRESH_NOT_FIXED = 10000;                     //In milliseconds.

//Temp
const long TEMP_REFRESH = 5000;                              //In milliseconds.

//LoRa
const int LORA_TX_POWER = 10;
const int LORA_TX_TIMEOUT = 10;
const int LORA_RX_TIMEOUT = 100;

///////////////////////////////////////////// State /////////////////////////////////////////////
#if (LOG_LEVEL & LOG_FLAG_INFO) > 0
#define LOG_INFO Serial.println
#else
#define LOG_INFO // macros
#endif

#if (LOG_LEVEL & LOG_FLAG_WARN) > 0
#define LOG_WARN Serial.println
#else
#define LOG_WARN // macros
#endif

#if (LOG_LEVEL & LOG_FLAG_ERR) > 0
#define LOG_ERR Serial.println
#else
#define LOG_ERR // macros
#endif

#if (LOG_LEVEL & LOG_FLAG_TIMING) > 0
#define LOG_TIMING Serial.println
#else
#define LOG_TIMING // macros
#endif

#if (LOG_LEVEL & LOG_FLAG_COMS) > 0
#define LOG_COMS Serial.println
#else
#define LOG_COMS // macros
#endif


SoftwareSerial gpsPort(GPS_TX_PIN, GPS_RX_PIN);
static NMEAGPS gps;
static gps_fix gpsFix;
float longitude = 0, latitude = 0;
ZMPT101B voltageSensor(VOLTAGE_SENSOR_PIN);

LiquidCrystal_I2C display(DISPLAY_ADDRESS, DISPLAY_WIDTH, DISPLAY_HEIGHT);

OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature tempSensor(&oneWire);
float temp = 0;
long lastTempCheck = 0;

SX127XLT lora;
char loraLocationBuffer[35];                                               //syntax-markers: 6;   id: 4;   lon: 4+dot+6;   lat: 4+dot+6;  null: 1;    blank: 2
char loraDataBuffer[32];                                                   //syntax-markers: 8;   id: 4;   state: 1;   voltage: 3+dot+1;  current: 3+dot+1;   temp: 3+dot+1;   null: 1;    blank: 3

const byte RECEIVE_BUFFER_SIZE = 13;                                        //Power msg      ->  synatx-markers: 5;   id: 4;   state: 1;   null: 1;    blank: 2;;
unsigned char loraReceiveBuffer[RECEIVE_BUFFER_SIZE];

long lastTick = 0;
long lastGpsCheck = 0;

bool isActive = false;

const long FRAME_LENGTH = 1000 / TICK_RATE;

///////////////////////////////////////////// Heleprs /////////////////////////////////////////////
// float to string. Conversion function courtesy of: Alan Boother, 2013-07-03
char* f2s(float f, int p) {
  char* pBuff;
  const int iSize = 5;
  static char sBuff[iSize][20];
  static int iCount = 0;
  pBuff = sBuff[iCount];
  if (iCount >= iSize - 1) {
    iCount = 0;
  }
  else {
    iCount++;                           // advance the counter
  }
  return dtostrf(f, 0, p, pBuff);       // call the library function
}

///////////////////////////////////////////// API /////////////////////////////////////////////
//Returns the voltage in volts, with a single decimal.
//Operating range: 0V - 250V
//Reading range: 15V - 250V
//Resolution: 0.2V
//Accuracy: 96.5%
float readVoltage() {
  float voltage = voltageSensor.getVoltageAC();
  if (voltage < 15) {
    return 0;
  } else if (voltage > 250) {
    return 250;
  } else {
    return voltage;
  }
}

//Returns the current in amps with a single decimal.
//Operating range: -100A - 100A
//Reading range: 10A - 100A   (The absolute of negative readings is returned.)
//Resolution: 1A
//Accuracy: 98%
float readCurrent() {
  float adc = analogRead(CURRENT_SENSOR_PIN);
  float acsVoltage = (BOARD_VCC / BOARD_ADC_RESOLUTION) * adc;
  acsVoltage = acsVoltage - (BOARD_VCC * 0.5) + CURRENT_SENSOR_GND_OFFSET;
  float current = acsVoltage / CURRENT_SENSOR_SENSITIVITY;
  if (current < 1) {
    return 0;
  } else if (current > 100) {
    return 100;
  } else {
    return current;
  }
}

//Returns the temprature in celsius, with a single decimal.
//Operating range: -55C - 125C
//Reading range: -55C - 125C
//Resolution: 0.0625C       (In the range [-10C, 85C]; sensitivity drops beyond this range.
//Accuracy: 0.5%
float readTemp() {
  tempSensor.requestTemperatures();
  float temp = tempSensor.getTempCByIndex(0);
  if (temp < -55) {
    return -55;
  } else if (temp > 125) {
    return 125;
  } else {
    return temp;
  }
}

//Turn the power station on or off.
void setState(bool state) {
  LOG_INFO("state from:\t" + String(isActive) + "\tto:\t" + state);
  isActive = state;
  digitalWrite(SSR_PIN, state);
}

//Read the current position. A clear view of the sky is required.
//Resolution: 5-7m
//A value of (0, 0) indicates that a fix was not obtained (usually due to sky obstruction).
void readPosition(float& lon, float& lat) {
  if (gps.available(gpsPort)) {
    gpsFix = gps.read();
    if (gpsFix.valid.location) {
      lon = gpsFix.longitude();
      lat = gpsFix.latitude();
    } else {
      lon = 0.0;
      lat = 0.0;
    }
  }
}

///////////////////////////////////////////// Network API /////////////////////////////////////////////
void transmitLocation(float lon, float lat) {
  sprintf(loraLocationBuffer, "geo:%i,%s,%s", NETWORK_ID, f2s(lon, 6), f2s(lat, 6));
  //sprintf(loraLocationBuffer, "geo:%i,%i,%i", NETWORK_ID, 2, 3);
  int length = sizeof(loraLocationBuffer);
  LOG_COMS(loraLocationBuffer);
  lora.transmitIRQ(loraLocationBuffer, length, LORA_TX_TIMEOUT, LORA_TX_POWER, NO_WAIT);
}

void transmitData(float voltage, float current, float temp) {
  sprintf(loraDataBuffer, "dat:%i,%i,%s,%s,%s", NETWORK_ID, isActive, f2s(voltage, 1), f2s(current, 1), f2s(temp, 1));
  int length = sizeof(loraDataBuffer);
  LOG_COMS(loraDataBuffer);
  lora.transmitIRQ(loraDataBuffer, length, LORA_TX_TIMEOUT, LORA_TX_POWER, NO_WAIT);
}

void handleRxError() {
  unsigned int irqStatus = lora.readIrqStatus();
  if (irqStatus & IRQ_RX_TIMEOUT) {
    //LOG_INFO(F("LoRa packet read timed out."));
  } else {
    LOG_WARN("WARN: LoRa packet read returned error code: " + String(irqStatus, HEX));
  }
}

void handleRxPacket() {
  //Couldn't use custom logging as Serial can't print unsigned char arrays.
#if (LOG_LEVEL & LOG_FLAG_INFO) > 0
  Serial.print("got packet: ");
  lora.printASCIIPacket(loraReceiveBuffer, RECEIVE_BUFFER_SIZE);
  Serial.println();
#endif
  String msg = String((char*) loraReceiveBuffer);
  if (msg.substring(0, 3) == "pwr") {
    int id = msg.substring(4, 5).toInt();
    if (id == NETWORK_ID) {
      bool newState = msg.substring(6, 7).toInt();
      setState(newState);
    }
  }
}
///////////////////////////////////////////// Timing /////////////////////////////////////////////
unsigned long debugTimer = 0;
void inline resetTimer() {
#ifdef RUN_PERFORMANCE_ANALYTICS
  debugTimer = millis();
#endif
}

void inline printTimer(String name, bool keepTime = false) {
#ifdef RUN_PERFORMANCE_ANALYTICS
  unsigned long duration = millis() - debugTimer;
  LOG_TIMING("timer [" + name + "]:" + duration + "ms");
  if (!keepTime) {
    debugTimer = millis();
  }
#endif
}


///////////////////////////////////////////// Core API /////////////////////////////////////////////
void setup() {
  //Init serial
#if (LOG_LEVEL > 0)
  Serial.begin(SERIAL_BAUDRATE);
#endif
  LOG_INFO(F("===== Power Grid Station 1.0.0 running! ====="));
  LOG_INFO(F("License: MIT"));
  LOG_INFO(F("Author: Dulfiqar 'Active Diamond' H. Al-Safi"));
  LOG_INFO("\nNetwork ID:" + String(NETWORK_ID));


  //Init pins.
  LOG_INFO(F("Initializing all pins."));
  pinMode(SSR_PIN, OUTPUT);
  digitalWrite(SSR_PIN, HIGH);

  pinMode(CURRENT_SENSOR_PIN, INPUT);

  //Init GPS.
  LOG_INFO(F("Initializing GPS."));
  gpsPort.begin(GPS_BAUDRATE);

  //Init voltage sensor.
  LOG_INFO(F("Initializing voltage sensor."));
  delay(10);
  voltageSensor.setSensitivity(VOLTAGE_SENSOR_SENSITIVITY);
  voltageSensor.calibrate();

  //Init temp sensor.
  LOG_INFO(F("Initializing temp sensor."));
  tempSensor.begin();

  //Init LoRa
  LOG_INFO(F("Initializing LoRa."));
  SPI.begin();
  if (!lora.begin(LORA_NSS_PIN, LORA_DEVICE)) {
    LOG_ERR(F("ERROR: LoRa not found!."));
    while (true) {};
  }
  delay(1000);
  LOG_INFO(F("Setting up LoRa."));
  lora.setupLoRa(434000000, 0, LORA_SF7, LORA_BW_125, LORA_CR_4_5, LDRO_AUTO);            //Configure frequency and LoRa settings.

  //Init display
  LOG_INFO(F("Initializing LCD display."));
  display.init();
  display.backlight();

  //Boot-up screen.
  display.clear();
  display.print("POWER GRID   A ");
  display.setCursor(1, 3);
  display.print("MONITOR    LAB");
  delay(BOOT_UP_SCREEN_DURATION);

  LOG_INFO(F("Initialization done."));
  LOG_INFO(F("===== ===== ====="));
}

void loop() {
  unsigned long startTime = millis();
  resetTimer();

  //Check for msgs
  byte length = lora.receiveIRQ(loraReceiveBuffer, RECEIVE_BUFFER_SIZE, LORA_RX_TIMEOUT, WAIT_RX);
  if (length == 0) {
    handleRxError();
  } else {
    handleRxPacket();
  }

  float voltage = readVoltage();
  printTimer("voltage");
  float current = readCurrent();
  printTimer("current");

  if (millis() - lastTempCheck > TEMP_REFRESH || millis() < lastTempCheck) {
    LOG_INFO(F("Updating temp."));
    resetTimer();
    temp = readTemp();
    lastTempCheck = millis();
    printTimer("temp");
  }

  resetTimer();
  int gpsRefresh = (longitude == 0 && latitude == 0) ? GPS_REFRESH_NOT_FIXED : GPS_REFRESH_FIXED;
  //Second condition is to account for overflow of millis. This is expected to happen after around 25 days of continous runtime.
  if (millis() - lastGpsCheck > gpsRefresh || millis() < lastGpsCheck) {
    LOG_INFO(F("Updating GPS."));
    readPosition(longitude, latitude);
    transmitLocation(longitude, latitude);
    lastGpsCheck = millis();
    printTimer("gps");
  } else {
    transmitData(voltage, current, temp);
    printTimer("transmit");
  }

  resetTimer();
  display.clear();
  display.print("V=");
  display.print(voltage, 1);
  display.print("V ");
  display.print("I=");
  display.print(current, 1);
  display.print("A ");


  display.setCursor(5, 1);
  display.print("T=");
  display.print(temp, 1);
  display.print("C");
  //display.print(longitude, 5);
  //display.print(", ");
  //display.print(latitude, 5);
  printTimer("display");

  //Limit tick rate.
  if (millis() < startTime) {       //Account for overflow.
    startTime = 0;
  }
  long sleepDuration = FRAME_LENGTH - (millis() - startTime);
  //LOG_TIMING.println("start\t\tnow\t\tsleepDur\t\tFRAME_LEN\n" + String(startTime) + "\t\t" + millis() + "\t\t" + sleepDuration + "\t\t" + FRAME_LENGTH);
  LOG_TIMING(sleepDuration);
  if (sleepDuration > 0) {
    delay(sleepDuration);
  }
}
