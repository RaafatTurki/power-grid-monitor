package main

import (
	"net/http"

	"power-grid-monitor/core/database"
	"power-grid-monitor/core/ws"
	"power-grid-monitor/lib/log"

	"github.com/gorilla/mux"
)

const ADDR = ":3000"

func main() {
	db := database.DBConnect(database.DB_NAME)
	database.DBMigrate(db)

	router := mux.NewRouter()
	router.HandleFunc("/ws", ws.WSHTTPHandler)
	router.PathPrefix("/").Handler(http.FileServer(http.Dir("./public")))

	log.PrintConsole(log.INFO, "Starting server on %s", ADDR)
	log.PanicErr(http.ListenAndServe(ADDR, router))
}
