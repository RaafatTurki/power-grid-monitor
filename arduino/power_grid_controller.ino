//power_grid_controller.ino
///////////////////////////////////////////// Libs /////////////////////////////////////////////
#include <SPI.h>
#include <SX127XLT.h>                           

#include <ArduinoWebsockets.h>
#include <WiFi.h>

///////////////////////////////////////////// Preproceser Flags /////////////////////////////////////////////
#define LOG_FLAG_INFO 1
#define LOG_FLAG_WARN 2
#define LOG_FLAG_ERR 4
#define LOG_FLAG_TIMING 8
#define LOG_FLAG_COMS 16

///////////////////////////////////////////// Hardware Setup /////////////////////////////////////////////
//Pins
const byte LORA_NSS_PIN = 15;
const byte LORA_MOSI_PIN = 13;
const byte LORA_MISO_PIN = 12;
const byte LORA_SCK_PIN = 14;

const byte LORA_DIO0_PIN = 2;
const byte LORA_NRESET_PIN = -1;
//Important: The LoRa is a 3.3V module!

//LoRa
#define LORA_DEVICE DEVICE_SX1278               

///////////////////////////////////////////// Config /////////////////////////////////////////////
//General
#define LOG_LEVEL 31     //(LOG_TIMING + LOG_COMS)
//#define RUN_PERFORMANCE_ANALYTICS true

const unsigned int SERIAL_BAUDRATE = 115200;
const unsigned int COOLDOWN = 10;                            //Reads and transmits per second.

const int LORA_TX_POWER = 10;
const long LORA_TX_TIMEOUT = 1000;                            //Only needed if WAIT_TX is used, instead of NO_WAIT
const long LORA_RX_TIMEOUT = 1000;                            //Only needed if WAIT_RX is used, instead of NO_WAIT

const char* WIFI_SSID = "ActiveDiamond";
const char* WIFI_PASSWORD = "Direwolf20";
const char* SERVER_URL = "wss://power-grid-monitor.potatolord2.repl.co/ws";

const long WIFI_CONNECTION_COOLDOWN = 100;
const long SERVER_CONNECTION_FAILED_COOLDOWN = 1000;

const int POWER_COMMAND_ATTEMPTS = 20;
const long POWER_COMMAND_COOLDOWN = 10;

///////////////////////////////////////////// Credentials /////////////////////////////////////////////
// certificate for https://power-grid-monitor.potatolord2.repl.co
// ISRG Root X1, valid until Mon Sep 15 2025, size: 1826 bytes 
const char* rootCACertificate = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIFFjCCAv6gAwIBAgIRAJErCErPDBinU/bWLiWnX1owDQYJKoZIhvcNAQELBQAw\n" \
"TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n" \
"cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjAwOTA0MDAwMDAw\n" \
"WhcNMjUwOTE1MTYwMDAwWjAyMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3Mg\n" \
"RW5jcnlwdDELMAkGA1UEAxMCUjMwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\n" \
"AoIBAQC7AhUozPaglNMPEuyNVZLD+ILxmaZ6QoinXSaqtSu5xUyxr45r+XXIo9cP\n" \
"R5QUVTVXjJ6oojkZ9YI8QqlObvU7wy7bjcCwXPNZOOftz2nwWgsbvsCUJCWH+jdx\n" \
"sxPnHKzhm+/b5DtFUkWWqcFTzjTIUu61ru2P3mBw4qVUq7ZtDpelQDRrK9O8Zutm\n" \
"NHz6a4uPVymZ+DAXXbpyb/uBxa3Shlg9F8fnCbvxK/eG3MHacV3URuPMrSXBiLxg\n" \
"Z3Vms/EY96Jc5lP/Ooi2R6X/ExjqmAl3P51T+c8B5fWmcBcUr2Ok/5mzk53cU6cG\n" \
"/kiFHaFpriV1uxPMUgP17VGhi9sVAgMBAAGjggEIMIIBBDAOBgNVHQ8BAf8EBAMC\n" \
"AYYwHQYDVR0lBBYwFAYIKwYBBQUHAwIGCCsGAQUFBwMBMBIGA1UdEwEB/wQIMAYB\n" \
"Af8CAQAwHQYDVR0OBBYEFBQusxe3WFbLrlAJQOYfr52LFMLGMB8GA1UdIwQYMBaA\n" \
"FHm0WeZ7tuXkAXOACIjIGlj26ZtuMDIGCCsGAQUFBwEBBCYwJDAiBggrBgEFBQcw\n" \
"AoYWaHR0cDovL3gxLmkubGVuY3Iub3JnLzAnBgNVHR8EIDAeMBygGqAYhhZodHRw\n" \
"Oi8veDEuYy5sZW5jci5vcmcvMCIGA1UdIAQbMBkwCAYGZ4EMAQIBMA0GCysGAQQB\n" \
"gt8TAQEBMA0GCSqGSIb3DQEBCwUAA4ICAQCFyk5HPqP3hUSFvNVneLKYY611TR6W\n" \
"PTNlclQtgaDqw+34IL9fzLdwALduO/ZelN7kIJ+m74uyA+eitRY8kc607TkC53wl\n" \
"ikfmZW4/RvTZ8M6UK+5UzhK8jCdLuMGYL6KvzXGRSgi3yLgjewQtCPkIVz6D2QQz\n" \
"CkcheAmCJ8MqyJu5zlzyZMjAvnnAT45tRAxekrsu94sQ4egdRCnbWSDtY7kh+BIm\n" \
"lJNXoB1lBMEKIq4QDUOXoRgffuDghje1WrG9ML+Hbisq/yFOGwXD9RiX8F6sw6W4\n" \
"avAuvDszue5L3sz85K+EC4Y/wFVDNvZo4TYXao6Z0f+lQKc0t8DQYzk1OXVu8rp2\n" \
"yJMC6alLbBfODALZvYH7n7do1AZls4I9d1P4jnkDrQoxB3UqQ9hVl3LEKQ73xF1O\n" \
"yK5GhDDX8oVfGKF5u+decIsH4YaTw7mP3GFxJSqv3+0lUFJoi5Lc5da149p90Ids\n" \
"hCExroL1+7mryIkXPeFM5TgO9r0rvZaBFOvV2z0gp35Z0+L4WPlbuEjN/lxPFin+\n" \
"HlUjr8gRsI3qfJOQFy/9rKIJR0Y/8Omwt/8oTWgy1mdeHmmjk7j1nYsvC9JSQ6Zv\n" \
"MldlTTKB3zhThV1+XWYp6rjd5JW1zbVWEkLNxE7GJThEUG3szgBVGP7pSWTUTsqX\n" \
"nLRbwHOoq7hHwg==\n" \
"-----END CERTIFICATE-----\n" \
"";

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


SX127XLT lora;
const byte RECEIVE_BUFFER_SIZE = 35;
unsigned char loraReceiveBuffer[RECEIVE_BUFFER_SIZE];              //Location msg   ->  syntax-markers: 6;   id: 4;   lon: 4+dot+6;   lat: 4+dot+6;  null: 1;    blank: 2;
                                                                   //Data msg       ->  syntax-markers: 8;   id: 4;   state: 1;   voltage: 3+dot+1;  current: 3+dot+1;   temp: 3+dot+1;   null: 1;    blank: 3;
unsigned char loraPowerCommandBuffer[13];                          //Power msg      ->  synatx-markers: 5;   id: 4;   state: 1;   null: 1;    blank: 2;
char websocketRegisterCommand[] = "reg:s";
using namespace websockets;
WebsocketsClient client;

///////////////////////////////////////////// Network API /////////////////////////////////////////////
void handleRxError() {
  unsigned int irqStatus = lora.readIrqStatus();
  if (irqStatus & IRQ_RX_TIMEOUT) {
    LOG_INFO(F("LoRa packet read timed out."));
  } else {
    LOG_WARN("WARN: LoRa packet read returned error code: " + String(irqStatus, HEX));
  }
}

void handleRxPacket() {
  //Couldn't use custom logging as Serial can't print unsigned char arrays.
#if (LOG_LEVEL & LOG_FLAG_INFO) > 0
  Serial.print("to ws: ");
  lora.printASCIIPacket(loraReceiveBuffer, RECEIVE_BUFFER_SIZE);
  Serial.println();
#endif
  client.send((char*)loraReceiveBuffer);
}

void connectToServer() {
  int succ;
  client.onMessage(onDummyMessage);
  client.onEvent(onDummyEvent);
  client.setCACert(rootCACertificate);
  do {
    succ = client.connect(SERVER_URL);
    if (succ) {
      LOG_COMS(F("Connected to websockets server successfully!"));
      client.onMessage(onWebsocketMessage);
      client.onEvent(onWebsocketEvent);
  
      client.send(websocketRegisterCommand);
    } else {
      LOG_WARN(F("Failed to connect to server, retrying!"));
      delay(SERVER_CONNECTION_FAILED_COOLDOWN);
    }
  } while (!succ);
}

void onWebsocketMessage(WebsocketsMessage msg) {
  LOG_COMS(F("Got msg\b"));
  LOG_COMS(msg.data());
  String data = msg.data(); 
  LOG_COMS(data.substring(0, 3));
  //LOG_COMS(data.substring(1, 4));
  if (data.substring(0, 3) == "pwr") {
    LOG_COMS(F("Got power command, echoing to station."));
    
    int length = sizeof(loraPowerCommandBuffer);
    data.toCharArray((char*) loraPowerCommandBuffer, length);
    for(int i = 0; i < POWER_COMMAND_ATTEMPTS; i++) {
      lora.transmit(loraPowerCommandBuffer, length, LORA_TX_TIMEOUT, LORA_TX_POWER, WAIT_TX);
      delay(POWER_COMMAND_COOLDOWN);
    }
  }
}

void onWebsocketEvent(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        LOG_COMS(F("Connnection opened."));
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        LOG_WARN(F("Connection to server lost!"));
        connectToServer();
    } else if(event == WebsocketsEvent::GotPing) {
        LOG_COMS(F("Got ping."));
    } else if(event == WebsocketsEvent::GotPong) {
        LOG_COMS(F("Got pong."));
    }
}

void onDummyMessage(WebsocketsMessage msg) {
LOG_INFO(F("Got dummy websocket message."));
}

void onDummyEvent(WebsocketsEvent event, String data) {
  LOG_INFO(F("Got dummy websocket event."));
}
///////////////////////////////////////////// Timing /////////////////////////////////////////////
unsigned long debugTimer = 0;
void resetTimer() {
#ifdef RUN_PERFORMANCE_ANALYTICS
  debugTimer = millis();
#endif 
}
void printTimer(String name, bool keepTime = false) {
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
  LOG_INFO(F("===== Power Grid Controller 1.0.0 running! ====="));
  LOG_INFO(F("License: MIT"));
  LOG_INFO(F("Author: Dulfiqar 'Active Diamond' H. Al-Safi"));
  
  //Init LoRa
  LOG_INFO(F("Initializing LoRa."));
  SPI.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_NSS_PIN);
  
  if (!lora.begin(LORA_NSS_PIN, LORA_NRESET_PIN, LORA_DIO0_PIN, LORA_DEVICE)) {
    LOG_ERR(F("ERROR: LoRa not found!."));
    while (true) {};
  }
  delay(1000);
  LOG_INFO(F("Setting up LoRa."));
  lora.setupLoRa(434000000, 0, LORA_SF7, LORA_BW_125, LORA_CR_4_5, LDRO_AUTO);            //Configure frequency and LoRa settings.

  //Init WiFi
  LOG_INFO(F("Initializing WiFi."));
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
#if (LOG_LEVEL & LOG_FLAG_INFO) > 0
    Serial.print(F("."));
#endif    
    delay(WIFI_CONNECTION_COOLDOWN);
  }
  LOG_INFO(F("WiFi connected!"));
  
  //Init WebSockets
  LOG_INFO(F("Initializing WebSockets."));
  connectToServer();
  
  LOG_INFO(F("Initialization done."));
  LOG_INFO(F("===== ===== ====="));
}

void loop() {
  byte length = lora.receive(loraReceiveBuffer, RECEIVE_BUFFER_SIZE, LORA_RX_TIMEOUT, WAIT_RX);
  if (length == 0) {
    handleRxError();
  } else {
    handleRxPacket();
  }
  client.poll();
  
  //Cooldown.
  delay(COOLDOWN);
}
