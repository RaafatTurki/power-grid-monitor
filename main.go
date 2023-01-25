package main

import (
	"net/http"

	"power-grid-monitor/core/ws"
	"power-grid-monitor/lib/log"

	"github.com/gorilla/mux"
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

const DB_NAME = "sqlite.db"
const ADDR = ":3000"

func main() {
	db, err := gorm.Open(sqlite.Open(DB_NAME), &gorm.Config{})
	if err != nil {
		log.PrintErr(err)
		return
	}

	db.AutoMigrate(&Station{})
	db.AutoMigrate(&StationState{})

	router := mux.NewRouter()
	router.HandleFunc("/ws", ws.WSHTTPHandler)
	router.PathPrefix("/").Handler(http.FileServer(http.Dir("./public")))

	log.PrintConsole(log.INFO, "Starting server on %s", ADDR)
	log.PanicErr(http.ListenAndServe(ADDR, router))
}
