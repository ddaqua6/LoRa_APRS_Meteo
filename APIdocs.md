/ - meteo stránka s WebSocket
/lora - dashboard stanice
/watch - stránka pro hodinky
/switch-meteo - meteo on/off
/switch-aprs - APRS-IS on/off
/change-aprsis - dialogové okno pro změnu APRS-IS serveru
/new-aprsis/newServer - nový APRS-IS server
/restart - restartuje desku
/tx/custPkt - vyšle packet
/reset-tx - reset t/o igate, meteo a upload na aktuální čas

/api/meteo - CSV meteo hodnot (teplota, tlak, vítr aktuální m/s, vítr dlouhodobý m/s, nárazy m/s)
/api/graphs-json - JSON meteo hodnot grafů, jako z /graphs
Příklad:
{
  "temperature": [
    -1.5,
    1.38,
    ...
    8.21,
    8.35
  ],
  "pressure": [
    1025,
    1025,
    ...
    1023,
    1023
  ],
  "wind": [
    1.75,
    1.5,
    ...
    1.25,
    1
  ]
}

/api/json - JSON kompletní přehled všech údajů ze stanice
Příklad:
{
	"general": {
		"version": 3.1,
		"system_time": 172848,
		"voltage": 4.14,
		"wifi_status": true,
		"wifi_signal_db": -45
	},
	"lora": {
		"meteo_callsign": "OK2DDS-6",
		"igate_callsign": "OK2DDS-1",
		"aprs_is_status": true,
		"aprs_is_server": "czech.aprs2.net",
		"hall_sensor": 2000,
		"last_rx": "OK2DDS-7"
	},
	"meteo": {
		"temperature": 10.0,
		"pressure": 1013,
		"actual_wind": 2.25,
		"long_period_wind": 0.5,
		"gust": 3.5,
		"min_temperature": -10,
		"max_temperature": 30,
		"min_pressure": 990,
		"max_pressure": 1020,
		"max_wind": 4,
		"max_gust": 6.5
	}
}