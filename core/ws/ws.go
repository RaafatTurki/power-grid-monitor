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

var registeredClientConns = []*websocket.Conn{}
var registeredStationConns = []*websocket.Conn{}

func isClientRegistered(conn *websocket.Conn, connsArr []*websocket.Conn) bool {
	for _, connClient := range connsArr {
		if connClient.RemoteAddr() == conn.RemoteAddr() {
			return true
		}
	}
	return false
}

func registerClient(conn *websocket.Conn, connsArr []*websocket.Conn) ([]*websocket.Conn, error) {
	if !isClientRegistered(conn, connsArr) {
		connsArr = append(connsArr, conn)
		log.PrintConsole(log.INFO, fmt.Sprintf("registered: %s", conn.RemoteAddr().String()))
		return connsArr, nil
	} else {
		return nil, errors.New(fmt.Sprintf("already registered: %s", conn.RemoteAddr().String()))
	}
}

func unregisterClient(conn *websocket.Conn, connsArr []*websocket.Conn) ([]*websocket.Conn, error) {
	connsClientsTemp := []*websocket.Conn{}
	isDone := false

	for _, registeredConn := range connsArr {
		if registeredConn.RemoteAddr() != conn.RemoteAddr() {
			connsClientsTemp = append(connsClientsTemp, registeredConn)
		} else {
			isDone = true
		}
	}

	if isDone {
		connsArr = connsClientsTemp
		log.PrintConsole(log.INFO, fmt.Sprintf("unregistered: %s", conn.RemoteAddr().String()))
		return connsArr, nil
	} else {
		return nil, errors.New(fmt.Sprintf("already unregistered: %s", conn.RemoteAddr().String()))
	}
}

func parseMsg(msgBytes []byte) (string, []string, error) {
	msg := string(msgBytes[:])
	msgParts := strings.Split(msg, ":")
	if len(msgParts) != 2 {
		return "", nil, errors.New(fmt.Sprintf("malformed message: %s", msg))
	} else {
		msgType := msgParts[0]
		msgArgs := strings.Split(msgParts[1], ",")
		return msgType, msgArgs, nil
	}
}

func HTTPWSHandler(w http.ResponseWriter, r *http.Request) {
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
		err = connHandler(conn, msg)
		if err != nil {
			log.PrintErr(err)
		}
	}
}

func connHandler(conn *websocket.Conn, msgBytes []byte) error {
	log.PrintConsole(log.DEBUG, "received message: %s", msgBytes)

	msgType, msgArgs, err := parseMsg(msgBytes)
	if err != nil {
		return err
	}

	switch msgType {
	case "":
		return errors.New(fmt.Sprintf("empty msg type"))
	case "reg":
		return msgHandlerReg(conn, msgArgs)
	case "unreg":
		return msgHandlerUnreg(conn, msgArgs)
	case "geo":
		return msgHandlerGeo(conn, msgArgs)
	case "dat":
		return msgHandlerDat(conn, msgArgs)
	case "pwr":
		return msgHandlerPwr(conn, msgArgs)
	case "regen":
		return msgHandlerRegen(conn, msgArgs)
	default:
		return errors.New(fmt.Sprintf("invalid msg type: \"%s\"", msgType))
	}
}

func msgHandlerReg(conn *websocket.Conn, msgArgs []string) error {
	if len(msgArgs) != 1 {
		return errors.New(fmt.Sprintf("wrong number of msg args: %s", msgArgs))
	}

	switch msgArgs[0] {
	case "":
		return errors.New(fmt.Sprintf("empty registration type"))
	case "c":
		connsArr, err := registerClient(conn, registeredClientConns)
		if err != nil {
			conn.WriteMessage(websocket.TextMessage, []byte(err.Error()))
		}
		registeredClientConns = connsArr
		return err
	case "s":
		connsArr, err := registerClient(conn, registeredStationConns)
		if err != nil {
			conn.WriteMessage(websocket.TextMessage, []byte(err.Error()))
		}
		registeredStationConns = connsArr
		return err
	default:
		return errors.New(fmt.Sprintf("invalid registration type"))
	}
}

func msgHandlerUnreg(conn *websocket.Conn, msgArgs []string) error {
	if len(msgArgs) != 1 {
		return errors.New(fmt.Sprintf("wrong number of msg args: %s", msgArgs))
	}

	switch msgArgs[0] {
	case "":
		return errors.New(fmt.Sprintf("empty registration type"))
	case "c":
		connsArr, err := unregisterClient(conn, registeredClientConns)
		if err != nil {
			conn.WriteMessage(websocket.TextMessage, []byte(err.Error()))
		}
		registeredClientConns = connsArr
		return err
	case "s":
		connsArr, err := unregisterClient(conn, registeredStationConns)
		if err != nil {
			conn.WriteMessage(websocket.TextMessage, []byte(err.Error()))
		}
		registeredClientConns = connsArr
		return err
	default:
		return errors.New(fmt.Sprintf("invalid registration type"))
	}
}

func msgHandlerGeo(conn *websocket.Conn, msgArgs []string) error {
	if len(msgArgs) != 3 {
		return errors.New(fmt.Sprintf("wrong number of msg args: %s", msgArgs))
	}
	id, err_id := strconv.ParseUint(msgArgs[0], 10, 32)
	lon, err_lon := strconv.ParseFloat(msgArgs[1], 32)
	lat, err_lat := strconv.ParseFloat(msgArgs[2], 32)
	if err_id != nil || err_lon != nil || err_lat != nil {
		return errors.New(fmt.Sprintf("invalid msg args: %s", msgArgs))
	}
	err := service.StationUpdateGeolocation(uint(id), float32(lat), float32(lon))
	if err != nil {
		return err
	} else {
		for _, connClient := range registeredClientConns {
			connClient.WriteMessage(websocket.TextMessage, []byte(fmt.Sprintf("geo:%d,%f,%f", id, lon, lat)))
		}
		return nil
	}
}

func msgHandlerDat(conn *websocket.Conn, msgArgs []string) error {
	if len(msgArgs) != 5 {
		return errors.New(fmt.Sprintf("wrong number of msg args: %s", msgArgs))
	}

	id_uint64, err_id := strconv.ParseUint(msgArgs[0], 10, 32)
	state_uint64, err_state := strconv.ParseUint(msgArgs[1], 10, 32)
	voltage_f64, err_voltage := strconv.ParseFloat(msgArgs[2], 32)
	current_f64, err_current := strconv.ParseFloat(msgArgs[3], 32)
	temp_f64, err_temp := strconv.ParseFloat(msgArgs[4], 32)

	if err_id != nil || err_state != nil || err_voltage != nil || err_current != nil || err_temp != nil || (state_uint64 != 0 && state_uint64 != 1) {
		return errors.New(fmt.Sprintf("invalid msg args: %s", msgArgs))
	}

	id := uint(id_uint64)
	voltage := float32(voltage_f64)
	current := float32(current_f64)
	temp := float32(temp_f64)
	err := service.StationStateInsertData(id, state_uint64 != 0, voltage, current, temp)

	if err != nil {
		return err
	} else {
		for _, connClient := range registeredClientConns {
			connClient.WriteMessage(websocket.TextMessage, []byte(fmt.Sprintf("dat:%d,%d,%f,%f,%f", id, state_uint64, voltage, current, temp)))
		}
		return nil
	}
}

func msgHandlerPwr(conn *websocket.Conn, msgArgs []string) error {
	if len(msgArgs) != 2 {
		return errors.New(fmt.Sprintf("wrong number of msg args: %s", msgArgs))
	}

	id_uint64, err_id := strconv.ParseUint(msgArgs[0], 10, 32)
	state_uint64, err_state := strconv.ParseUint(msgArgs[1], 10, 32)

	if err_state != nil || err_id != nil {
		return errors.New(fmt.Sprintf("invalid msg args: %s", msgArgs))
	}

	for _, connStation := range registeredStationConns {
		connStation.WriteMessage(websocket.TextMessage, []byte(fmt.Sprintf("pwr:%d,%d", id_uint64, state_uint64)))
	}
	return nil
}

func msgHandlerRegen(conn *websocket.Conn, msgArgs []string) error {
	// service.RegenStateDataFile(1000, "./public/datasheet.xlsx")
	service.RegenStateDataFile(1000, "./web_front/build/datasheet.xlsx")
  conn.WriteMessage(websocket.TextMessage, []byte(fmt.Sprintf("regen:done")))
	return nil
}
