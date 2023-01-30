<script lang="ts">
  import store from './store.js'

  let stations = store.stations
  let stations_state = store.stations_state

  let column_data = [
    { title: "ID", icon: 'i-tabler-hash' },
    { title: "Power", icon: 'i-ic-round-power-settings-new' },
    { title: "Voltage", icon: 'i-tabler-circuit-voltmeter' },
    { title: "Current", icon: 'i-tabler-circuit-ammeter' },
    { title: "Temp", icon: 'i-tabler-temperature' },
    { title: "Lon", icon: 'i-mdi-longitude' },
    { title: "Lat", icon: 'i-mdi-latitude' },
  ]

  function on_power_btn_click(id: number, state: number) {
    store.socket_send(`pwr:${id},${state == 0 ? 1 : 0}`)
  }
</script>


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
        <!-- <p>{id}</p> -->
        <!-- <p>{dat}</p> -->
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
          <td>{d}</td>
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
