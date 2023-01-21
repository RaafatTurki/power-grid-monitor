package main

import (
	"fmt"
	"log"
	"net/http"

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

const ADDR = ":3000"

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool { return true },
}

func main() {
	db, err := gorm.Open("sqlite3", "sqlite.db")
	if err != nil {
		fmt.Println(err)
		return
	}

	defer db.Close()
	db.AutoMigrate(&Station{})
	db.AutoMigrate(&StationState{})

	router := mux.NewRouter()
	router.HandleFunc("/ws", func(w http.ResponseWriter, r *http.Request) {
		conn, err := upgrader.Upgrade(w, r, nil)
		if err != nil {
			log.Println("Failed to set websocket upgrade:", err)
			return
		}

		for {
			_, msg, err := conn.ReadMessage()
			if err != nil {
				log.Println("Failed to read message:", err)
				break
			}

			if wsHandler(conn, msg) != nil {
				break
			}
		}
	})
	router.PathPrefix("/").Handler(http.FileServer(http.Dir("./public")))

	log.Printf("Starting server on %s\n", ADDR)
	log.Fatal(http.ListenAndServe(ADDR, router))
}

func wsHandler(conn *websocket.Conn, msg []byte) error {

	// log.Printf("Received message: %s", msg)

	// err := conn.WriteMessage(websocket.TextMessage, msg)
	// if err != nil {
	// 	log.Println("Failed to write message:", err)
	// }
	return nil
}
