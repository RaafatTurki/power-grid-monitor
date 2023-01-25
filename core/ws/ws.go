package ws

import (
  "net/http"
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
      break
    }
  }
}


func WSHandler(conn *websocket.Conn, msg []byte) error {
  log.PrintConsole(log.INFO, "Received message: %s", msg)

  err := conn.WriteMessage(websocket.TextMessage, msg)
  if err != nil {
    log.PrintConsole(log.ERR, "Failed to write message: %s", err)
  }
  return nil
}
