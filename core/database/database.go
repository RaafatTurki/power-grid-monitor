package database

import (
	"power-grid-monitor/lib/log"

	"gorm.io/driver/sqlite"
	"gorm.io/gorm"
)

type Station struct {
	ID  uint `gorm:"primary_key"`
	Lon float32
	Lat float32
}

type StationState struct {
	gorm.Model
	StationID uint `gorm:"foreignkey:StationID;association_foreignkey:ID"`
	State     bool
	Temp      float32
	Current   float32
	Voltage   float32
}

func DBConnect(db_name string) *gorm.DB {
	db, err := gorm.Open(sqlite.Open(db_name), &gorm.Config{})
	if err != nil {
		log.PanicErr(err)
	}
	return db
}

func DBMigrate(db *gorm.DB) {
	db.AutoMigrate(&Station{})
	db.AutoMigrate(&StationState{})
}
