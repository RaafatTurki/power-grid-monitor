package ws

import (
	"errors"
	"fmt"
	"net/http"
	"strconv"
	"strings"

	"power-grid-monitor/core/service"
	"power-grid-monitor/lib/log"

	"github.com/gorilla/websocket"
)

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool { return true },
}

func WSHTTPHandler(w http.ResponseWriter, r *http.Request) {
	// upgrade the http request to a ws connection
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.PrintErr(err)
		return
	}

	// read incoming message
	for {
		_, msg, err := conn.ReadMessage()
		if err != nil {
			log.PrintErr(err)
			break
		}

		// handle message
		err = WSHandler(conn, msg)
		if err != nil {
			log.PrintErr(err)
			// break
		}
	}
}

func WSHandler(conn *websocket.Conn, msgBytes []byte) error {
	log.PrintConsole(log.DEBUG, "Received message: %s", msgBytes)

	msg := string(msgBytes[:])
	msgParts := strings.Split(msg, ":")
	if len(msgParts) != 2 {
		return errors.New(fmt.Sprintf("Malformed message: %s", msg))
	}
	msgType := msgParts[0]
	msgArgs := strings.Split(msgParts[1], ",")

	switch msgType {
	case "":
		return errors.New(fmt.Sprintf("No msg type: \"%s\"", msg))
	case "g":
		if len(msgArgs) != 3 {
			return errors.New(fmt.Sprintf("Wrong number of msg args: %s", msg))
		}
		id, err_id := strconv.ParseUint(msgArgs[0], 10, 32)
		lon, err_lon := strconv.ParseFloat(msgArgs[1], 32)
		lat, err_lat := strconv.ParseFloat(msgArgs[2], 32)
		if err_id != nil || err_lon != nil || err_lat != nil {
			return errors.New(fmt.Sprintf("Invalid msg args: %s", msg))
		}
		err := service.StationUpdateGeolocation(uint(id), float32(lat), float32(lon))
		if err != nil {
			return err
		}
	case "d":
		if len(msgArgs) != 5 {
			return errors.New(fmt.Sprintf("Wrong number of msg args: %s", msg))
		}
		id, err_id := strconv.ParseUint(msgArgs[0], 10, 32)
		state, err_state := strconv.ParseUint(msgArgs[1], 10, 32)
		voltage, err_voltage := strconv.ParseFloat(msgArgs[2], 32)
		current, err_current := strconv.ParseFloat(msgArgs[3], 32)
		temp, err_temp := strconv.ParseFloat(msgArgs[4], 32)
		if err_id != nil || err_state != nil || err_voltage != nil || err_current != nil || err_temp != nil || (state != 0 && state != 1) {
			return errors.New(fmt.Sprintf("Invalid msg args: %s", msg))
		}
		err := service.StationInsertData(uint(id), state != 0, float32(voltage), float32(current), float32(temp))
		if err != nil {
			return err
		}
	default:
		return errors.New(fmt.Sprintf("Invalid msg type: \"%s\"", msg))
	}

	return nil
}
