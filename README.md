# LoRa_APRS_Meteo
LoRa APRS Meteostation, IGate and Digipeater firmware for LilyGO TTGO<br><br>
Features: measures temperature, pressure (with BMP280) and wind (with hall sensor anemometer - KY-024), sends meteodata via LoRa APRS, via APRS-IS and/or via own GET requests. Works as a standard LoRa APRS IGate and/or digipeater, reports coverage.<br><br>
Source code is in src/main.cpp, before running your station please check:<br>
- you have a valid HAM radio license
- you have edited <b>config.h</b> and wrote your own configuration
<br><br>
If you need help or have any questions or suggestions, please reach me: david(at)ok2dds.cz
<h2>Setup guide</h2>
1. Download this repository and open it in PlatformIO. It should obtain all dependencies automatically, otherwise they are listed in platformio.ini.<br>
2. Edit include/config.h.<br>
3. Upload to your TTGO desk<br><br>
For running the temperature/pressure measurement, you will need to use BMP280 sensor. Please solder VCC to 3.3V pin, GND to GND, SCL to IO22 and SDA to IO21.<br>
For wind measurement, you will have to obtain an anemometer with magnets in it (suggest 3D printed) and KY-024 hall sensor in the anemometer base. Please solder + to 3.3V pin, G to GND and AO to IO35 (you can use also other pins, if you modify the config file). BMP280 often needs a calibration, please write correct offsets to config file. If you don't want to measure wind, turn USE_ANEMOMETER to false. Temperature/pressure sensor is detected automatically, so simply don't connect BMP280 if you don't want to measure temp./pressure.<br>
<h2>Config file documentation</h2>
Use_WiFi true/false - whether to connect to Wi-Fi (enables functions that need internet)<br>
ssid, password, Hostname - insert your Wi-Fi details and desired hostname (name, under which you will see the station in the LAN)<br><br>
USE_METEO true/false - enables LoRa APRS meteo beacons<br>
METEO_BEACON (in minutes), METEO_LAT, METEO_LON, METEO_COMMENT - details for meteo APRS beacon (WX station)<br>
USE_METEO_STATUS, METEO_STATUS - if you want to air link to your website or other text via status beacon<br>
GRAPH_LIMIT - how many values should be stored for weather charts. (suggested keeping default)<br>
USE_ANEMOMETER true/false - if you want to use wind meter, turn on<br>
Next values are for anemometer calibration. See comments in config.h.<br>
BMP_OFFSET - calibration of BMP280<br><br>
USE_IGATE - send received packets to APRS-IS, needs Wi-Fi turned on<br>
Next values are IGate and APRS-IS settings, please insert <b>your callsign</b> and other values!<br>
USE_DIGIPEATER - enable digipeating received packets with WIDE1-1 inside<br>
USE_LASTRX_STATUS - will send a status message via APRS-IS so you can see your station coverage<br>
HTTP GET DATA UPLOAD - for working with your own systems, will send a GET request with meteo values to the URL you specify every x minutes. (keep default - off=false)<br>
Please keep all settings you don't understand or need to use, false. Do not use experimental settings as you might overload your local LoRa network with it.<br>
<h2>Meteostation Apple Shortcut for your iPhone/Apple Watch</h2>
If you have iPhone, you can download and setup LoRa_APRS_Meteo.shortcut file and setup it on your phone. Download the shortcut file to your iPhone (you need to have installed the Shortcuts app), tap 'Setup shortcut', replace '192.168.1.1' with your meteostation's IP. Then add the shortcut to your iPhone's desktop to get fresh meteo information quickly and easily. The shortcut also works at Apple Watch, so you can have your meteostation also on your wrist!
<h2>API documentation</h2>
If the meteostation is connected to Wi-Fi, it runs a tiny webserver. Some of the endpoints are even suitable for browser use.<br>Access your station dashboard with entering your station IP to your browser. For accessing from outside your home network, make sure you open ports 80 and 5028 (websocket).<br>Documentation of GET requests (API endpoints) is also in APIdocs.txt<br><br>
<b>API endpoints with HTML webpage output (with previews)</b><br>
/ - meteo dashboard with WebSocket (the values update automatically) - WebSocket uses port 5028<br><br>
<img src="https://github.com/ddaqua6/LoRa_APRS_Meteo/blob/main/img/meteo.png"><br>
/lora - station dashboard<br><br>
<img src="https://github.com/ddaqua6/LoRa_APRS_Meteo/blob/main/img/dashboard.png"><br>
/graphs - charts of weather history<br><br>
<img src="https://github.com/ddaqua6/LoRa_APRS_Meteo/blob/main/img/charts.png"><br>
/watch - meteo dashboard without WebSocket suitable for small displays, e.g. smartwatch<br><br>
<b>API command endpoints</b><br>
/switch-meteo - meteo APRS beacon on/off<br>
/switch-aprs - APRS-IS on/off<br>
/new-aprsis/newServer - change APRS-IS server<br>
/restart - station restart (all stored values, as minimal and maximal temperature etc. will be lost)<br>
/tx/custPkt - sends custom packet via LoRa<br>
/reset-tx - resets timeouts<br><br>
<b>API endpoints suitable for working with your projects providing data in machine-readable format</b><br>
/api/meteo - CSV of meteo values<br>
/api/graphs-json - JSON of history of meteo values (same as on /graphs but only values in JSON)<br>
Example:
<code>
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
</code>
/api/json - complete JSON of all values from your station
Example:
<code>
{
	"general": {
		"version": "4.0",
		"destcall": "APLGM4",
		"system_time": 172848,
		"voltage": 4.14,
		"battery": 70,
		"wifi_status": true,
		"wifi_signal_db": -45,
		"wifi_ssid": "Home Wi-Fi",
		"wifi_hostname": "LORAMETEO",
		"bmp280_status": true
	},
	"lora": {
		"meteo_callsign": "OK2DDS-6",
		"meteo_enabled": true,
		"igate_callsign": "OK2DDS-1",
		"aprs_is_enabled": true,
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
</code>
