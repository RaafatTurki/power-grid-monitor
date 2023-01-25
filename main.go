package main

import (
	"net/http"

	"power-grid-monitor/core/log"

	"github.com/gorilla/mux"
	"github.com/gorilla/websocket"
	"github.com/jinzhu/gorm"
	_ "github.com/jinzhu/gorm/dialects/sqlite"
)

type Station struct {
	ID  uint `gorm:"primary_key"`
	Lon float32
	Lat float32
}

type StationState struct {
	gorm.Model
	StationID uint `gorm:"foreignkey:StationID;association_foreignkey:ID"`
	Temp      float32
	Current   float32
	Voltage   float32
}

const DB_NAME = "sqlite.db"
const ADDR = ":3000"

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool { return true },
}

func main() {
	db, err := gorm.Open("sqlite3", DB_NAME)
	if err != nil {
		log.PrintErr(err)
		return
	}

	defer db.Close()
	db.AutoMigrate(&Station{})
	db.AutoMigrate(&StationState{})

	router := mux.NewRouter()
	router.HandleFunc("/ws", func(w http.ResponseWriter, r *http.Request) {
		conn, err := upgrader.Upgrade(w, r, nil)
		if err != nil {
			log.PrintErr(err)
			return
		}

		for {
			_, msg, err := conn.ReadMessage()
			if err != nil {
				log.PrintErr(err)
				break
			}

			if wsHandler(conn, msg) != nil {
				break
			}
		}
	})
	router.PathPrefix("/").Handler(http.FileServer(http.Dir("./public")))

	log.PrintConsole(log.INFO, "Starting server on %s", ADDR)
	log.PanicErr(http.ListenAndServe(ADDR, router))
}

func wsHandler(conn *websocket.Conn, msg []byte) error {

	log.PrintConsole(log.INFO, "Received message: %s", msg)

	err := conn.WriteMessage(websocket.TextMessage, msg)
	if err != nil {
		log.PrintConsole(log.ERR, "Failed to write message: %s", err)
	}
	return nil
}
