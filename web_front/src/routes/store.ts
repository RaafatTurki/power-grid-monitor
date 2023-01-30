import { writable, type Writable } from 'svelte/store'

const BACKEND_WS_URL = 'wss://power-grid-monitor.potatolord2.repl.co/ws'

const stations: Writable<Map<number, number[]>> = writable(new Map())
const stations_state: Writable<Map<number, number[]>> = writable(new Map())

const socket = new WebSocket(BACKEND_WS_URL)

function msg_handler_dat(msg_args: string[]) {
  if (msg_args.length != 5) return
  let id = Number(msg_args[0])
  let state = Number(msg_args[1])
  let voltage = Number(msg_args[2])
  let current = Number(msg_args[3])
  let temp = Number(msg_args[4])

  if (isNaN(id) || isNaN(state) || isNaN(voltage) || isNaN(current) || isNaN(temp)) return
  if (state !== 1 && state !== 0) return

  const dat = [state, voltage, current, temp]
  
  stations_state.update(obj => obj.set(id, dat))
}

function msg_handler_geo(msg_args: string[]) {
  if (msg_args.length != 3) return
  let id = Number(msg_args[0])
  let lon = Number(msg_args[1])
  let lat = Number(msg_args[2])

  if (isNaN(id) || isNaN(lon) || isNaN(lat)) return

  const geo = [lon, lat]

  stations.update(obj => obj.set(id, geo))
}

function parse_ws_msg(msg: string) {
  let msg_arr = msg.split(':')
  if (msg_arr.length != 2) return

  let msg_type = msg_arr[0]
  let msg_args = msg_arr[1].split(',')

  switch(msg_type) {
    case "dat":
      msg_handler_dat(msg_args)
    break;
    case "geo":
      msg_handler_geo(msg_args)
    break;
    default:
      console.error(`unknown msg_type ${msg_type}`)
  }
}

socket.addEventListener('open', (_e) => {
  socket.send('reg:c')
})

socket.addEventListener('message', (e) => {
  parse_ws_msg(e.data)
  // console.log(e.data)
  // messageStore.set(e.data)
})

// const sendMessage = (message: string) => {
//   if (socket.readyState <= 1) {
//     socket.send(message);
//   }
// }

function socket_send(msg: string) {
  socket.send(msg)
}

export default {
  stations,
  stations_state,
  socket_send,
}
