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

var connsClients = []*websocket.Conn{}

func isClientRegistered(conn *websocket.Conn) bool {
	for _, connClient := range connsClients {
		if connClient.RemoteAddr() == conn.RemoteAddr() {
			return true
		}
	}
	return false
}

func registerClient(conn *websocket.Conn) error {
	connsClients = append(connsClients, conn)
	return nil
}

func unregisterClient(conn *websocket.Conn) error {
	connsClientsTemp := []*websocket.Conn{}

	for _, connClient := range connsClients {
		if connClient.RemoteAddr() != conn.RemoteAddr() {
			connsClientsTemp = append(connsClientsTemp, connClient)
		}
	}

	connsClients = connsClientsTemp
	return nil
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
	case "reg":
		if isClientRegistered(conn) {
			conn.WriteMessage(websocket.TextMessage, []byte("Client already registered"))
			log.PrintConsole(log.DEBUG, fmt.Sprintf("Re-registering client attempt: %s", conn.RemoteAddr().String()))
		} else {
			registerClient(conn)
			conn.WriteMessage(websocket.TextMessage, []byte("Registered client"))
			log.PrintConsole(log.DEBUG, fmt.Sprintf("Registering client: %s", conn.RemoteAddr().String()))
		}
	case "unreg":
		if isClientRegistered(conn) {
			unregisterClient(conn)
			conn.WriteMessage(websocket.TextMessage, []byte("Unregistered client"))
			log.PrintConsole(log.DEBUG, fmt.Sprintf("Unregistering client: %s", conn.RemoteAddr().String()))
		} else {
			conn.WriteMessage(websocket.TextMessage, []byte("Client already unregistered"))
			log.PrintConsole(log.DEBUG, fmt.Sprintf("Unregistering unregistered client attempt: %s", conn.RemoteAddr().String()))
		}
	case "geo":
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
	case "dat":
		if len(msgArgs) != 5 {
			return errors.New(fmt.Sprintf("Wrong number of msg args: %s", msg))
		}
		id_uint64, err_id := strconv.ParseUint(msgArgs[0], 10, 32)
		state_uint64, err_state := strconv.ParseUint(msgArgs[1], 10, 32)
		voltage_f64, err_voltage := strconv.ParseFloat(msgArgs[2], 32)
		current_f64, err_current := strconv.ParseFloat(msgArgs[3], 32)
		temp_f64, err_temp := strconv.ParseFloat(msgArgs[4], 32)
		if err_id != nil || err_state != nil || err_voltage != nil || err_current != nil || err_temp != nil || (state_uint64 != 0 && state_uint64 != 1) {
			return errors.New(fmt.Sprintf("Invalid msg args: %s", msg))
		}
		id := uint(id_uint64)
		voltage := float32(voltage_f64)
		current := float32(current_f64)
		temp := float32(temp_f64)
		err := service.StationInsertData(id, state_uint64 != 0, voltage, current, temp)
		if err != nil {
			return err
		}
		for _, connClient := range connsClients {
			connClient.WriteMessage(websocket.TextMessage, []byte(fmt.Sprintf("f:%d,%d,%f,%f,%f", id, state_uint64, voltage, current, temp)))
		}
	default:
		return errors.New(fmt.Sprintf("Invalid msg type: \"%s\"", msg))
	}

	return nil
}
