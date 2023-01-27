package service

import (
	. "power-grid-monitor/core/database"
	"power-grid-monitor/lib/log"
)

var db = DBConnect(DB_NAME)

func StationUpdateGeolocation(id uint, lon float32, lat float32) error {
	station := Station{ID: id, Lon: lon, Lat: lat}
	err := db.Save(&station).Error
	if err != nil {
		return err
	} else {
		log.PrintConsole(log.DEBUG, "Updating geolocation")
		return nil
	}
}

func StationInsertData(id uint, state bool, voltage float32, current float32, temp float32) error {
	stationState := StationState{
		StationID: id,
		State:     state,
		Temp:      temp,
		Current:   current,
		Voltage:   voltage,
	}

	err := db.Create(&stationState).Error
	if err != nil {
		return err
	} else {
		log.PrintConsole(log.DEBUG, "Inserting Data")
		return nil
	}
}
