package service

import (
	. "power-grid-monitor/core/database"
	"power-grid-monitor/core/excel"
	"power-grid-monitor/lib/log"
)

var db = DBConnect(DB_NAME)

func StationUpdateGeolocation(id uint, lon float32, lat float32) error {
	station := Station{ID: id, Lon: lon, Lat: lat}
	err := db.Save(&station).Error
	if err != nil {
		return err
	} else {
		log.PrintConsole(log.DEBUG, "station geolocation updated")
		return nil
	}
}

func StationStateInsertData(id uint, state bool, voltage float32, current float32, temp float32) error {
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
		log.PrintConsole(log.DEBUG, "station state data inserted")
		return nil
	}
}

func RegenStateDataFile(count int, path string) {
	file, main_sheet_name := excel.CreateFile()

	var station_states []StationState
	db.Order("created_at desc").Limit(count).Find(&station_states)

  excel.AddTitlesRow(file, main_sheet_name, 1, []string{"ID", "State", "Voltage", "Current", "Temprature", "Date", "Timestamp (ms)"})

	for i, ss := range station_states {
    var state_uint uint = 0
    if ss.State {
      state_uint = 1
    }
    // i+2 bcause excel files are 1 indexed and because the 1st row is reserved for titles
		excel.AddStationStateRow(file, main_sheet_name, i+2, ss.StationID, state_uint, ss.Voltage, ss.Current, ss.Temp, ss.CreatedAt)
	}

	err := file.SaveAs(path)
	log.PrintErr(err)
}

