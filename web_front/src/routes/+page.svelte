<script lang="ts">
  import store from './store.js'

  let stations = store.stations
  let stations_state = store.stations_state

  let column_data = [
    { title: "ID",      unit: '',     icon: 'i-tabler-hash' },
    { title: "Power",   unit: '',     icon: 'i-ic-round-power-settings-new' },
    { title: "Voltage", unit: 'v',    icon: 'i-tabler-circuit-voltmeter' },
    { title: "Current", unit: ' amp', icon: 'i-tabler-circuit-ammeter' },
    { title: "Temp",    unit: '°',    icon: 'i-tabler-temperature' },
    { title: "Lon",     unit: '',     icon: 'i-mdi-longitude' },
    { title: "Lat",     unit: '',     icon: 'i-mdi-latitude' },
  ]

  function on_power_btn_click(id: number, state: number) {
    store.socket_send(`pwr:${id},${state == 0 ? 1 : 0}`)
  }

  function on_download_btn_click() {
    store.socket_send(`regen:`)
  }
</script>

<button
  on:click={() => { on_download_btn_click() }}>
  Download Datasheet
  <span class="i-material-symbols-download">kk</span>
</button>

<table>
  <tr>
    {#each column_data as col}
      <th>
        <span class={col.icon}>kk</span>
        {col.title}
      </th>
    {/each}
  </tr>

  {#each [...$stations_state] as station_state}
    {@const id = station_state[0]}
    {@const dat = station_state[1]}
    <tr>
      <td>{id}</td>
      {#each dat as d, i}
        {#if i == 0}
          <td>
            <button
              on:click={() => { on_power_btn_click(id, dat[0]) }}
              class={d ? 'bg-green' : 'bg-red'}>
              Power
            </button>
          </td>
        {:else}
          <td>{d}{column_data[i+1].unit}</td>
        {/if}
      {/each}

      {#if true}
        {@const lonlat = $stations.get(id)}

        {#if lonlat}
          <td>{lonlat[0]}</td>
          <td>{lonlat[1]}</td>
        {:else}
          <td>N/A</td>
          <td>N/A</td>
        {/if}

      {/if}
    </tr>
  {/each}
</table>
