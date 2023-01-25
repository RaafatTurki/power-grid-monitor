package service

import "power-grid-monitor/lib/log"

func StationUpdateGeolocation(id uint, lon float32, lat float32) error {
	log.PrintConsole(log.INFO, "Updating geolocation")
	return nil
}

func StationInsertData(id uint, state bool, voltage float32, current float32, temp float32) error {
	log.PrintConsole(log.INFO, "Inserting Data")
	return nil
}
