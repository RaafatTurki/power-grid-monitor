import { writable, type Writable } from 'svelte/store'

const WS_URL = 'wss://power-grid-monitor.potatolord2.repl.co'
const HTTP_URL = 'https://power-grid-monitor.potatolord2.repl.co'
// const WS_URL = 'ws://localhost:3000'
// const HTTP_URL = 'http://localhost:3000'

const stations: Writable<Map<number, number[]>> = writable(new Map())
const stations_state: Writable<Map<number, number[]>> = writable(new Map())

const socket = new WebSocket(`${WS_URL}/ws`)

function msg_handler_dat(msg_args: string[]) {
  if (msg_args.length != 5) return
  let id = Number(msg_args[0])
  let state = Number(msg_args[1])
  let voltage = Number(Number(msg_args[2]).toFixed(3))
  let current = Number(Number(msg_args[3]).toFixed(3))
  let temp = Number(Number(msg_args[4]).toFixed(3))

  if (isNaN(id) || isNaN(state) || isNaN(voltage) || isNaN(current) || isNaN(temp)) return
  if (state !== 1 && state !== 0) return

  const dat = [state, voltage, current, temp]
  
  stations_state.update(obj => obj.set(id, dat))
}

function msg_handler_geo(msg_args: string[]) {
  if (msg_args.length != 3) return
  let id = Number(msg_args[0])
  let lon = Number(Number(msg_args[1]).toFixed(5))
  let lat = Number(Number(msg_args[2]).toFixed(5))

  if (isNaN(id) || isNaN(lon) || isNaN(lat)) return

  const geo = [lon, lat]

  stations.update(obj => obj.set(id, geo))
}

function msg_handler_regen(msg_args: string[]) {
  if (msg_args.length != 1) return
  if (msg_args[0] != "done") return
  
  var link = document.createElement('a')
  link.download = "datasheet.xlsx"
  link.href = `${HTTP_URL}/datasheet.xlsx`
  link.click()
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
    case "regen":
      msg_handler_regen(msg_args)
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


function socket_send(msg: string) {
  socket.send(msg)
}

export default {
  stations,
  stations_state,
  socket_send,
}
