// GENERAL SETTINGS

#define Use_WiFi true                   // enable Wi-Fi connection and HTTP web server
const char* ssid = "SSID";              // WiFi SSID
const char* password = "mywifipassword";		// WiFi Password
#define Hostname "LORAMETEO"         // Hostname, name of computer visible in the network

// METEO SERVICE SETTINGS

#define USE_METEO true                 // beacon meteo packets and store values for graphs
#define METEO_CALLSIGN "N0CALL-6"
#define METEO_BEACON 15
#define METEO_LAT "4000.00N"
#define METEO_LON "01600.00E"
#define METEO_COMMENT "LoRa IGate&Meteo"
#define STATION_ALTITUDE 235            // station altitude for weather calculations (meters ASL)
#define USE_METEO_STATUS false           // send status below in timeout of igate packet (needs wifi, igate on and aprs-is)
#define METEO_STATUS "e.g. your website link"
#define GRAPH_LIMIT 144                   // how many values to store for graphs (too high can cause errors)

#define USE_ANEMOMETER false                     // turn on/off wind meter
#define HALL_SENSOR_PIN 35                 // use only ADC_1 pins
#define ANEMOMETER_LENGTH 0.25           // how long distance (meters) is done by spinning anemometer from one magnet to the next one
#define ANEMO_RECALC_LIMIT 2            // calibrate hall sensor
#define ANEMO_AC_THRESHOLD 1830          // analog value threshold to trigger magnet detection
#define ANEMO_AC_LOSE      2000          // analog value threshold - magnet is away
#define ANEMO_RECALC_LIMIT_TIMEOUT 900  // auto update long-period speed after x seconds (should be METEO_BEACON * 60)
#define ANEMO_RECALC_ACTUAL_SPEED 4    // set actual wind speed to 0 if anemometer is not spinning for x seconds

#define USE_PMSENSOR false            // turn on/off particle matter sensor
#define PMSENSOR_CLEAN_DAYS 4        // auto clean the sensor each x days (default 4)
#define PMSENSOR_CLEAN_ON_STARTUP true // startup takes more time, but recommended to clean it

#define BMP_OFFSET_TEMP -1          // calibrate your BMP sensor (these values will be added to values measured by BMP for calibration)
#define BMP_OFFSET_PRESS 28

// IGATE SERVICE SETTINGS

#define Use_IGATE true              // send received packets to APRS-IS, needs Wi-Fi turned on
                                    // Igate can be turned off with /switch-aprs, but can not be turned on if it's disabled by this
#define IGATE_BEACON 59
#define IGATE_BCN_NETWORK true              // if true, will send igate beacon via aprs-is, if false will send via LoRa
#define IGATE_LAT "4000.00N"
#define IGATE_LON "01600.00E"
#define IGATE_COMMENT "LoRa IGate&Meteo"
#define IGATE_CALLSIGN "N0CALL-1"
#define APRS_IS_Server "rotate.aprs2.net"    // APRS-IS server for igate
#define APRS_IS_Port 14580                  // server port, keep default (14580)
#define APRS_IS_Password 10000              // Your APRS-IS password

#define USE_DIGIPEATER true                 // allow digipeating of packets
#define USE_LASTRX_STATUS true              // display Last RX status on igate

// HTTP GET DATA UPLOAD SERVICE SETTINGS (experimental, for development)

#define Use_UPLOAD false
#define SERVER_URL "http://meteo.mywebsite.com/update.php?values=" // can be used for working with your SQL database
#define UPLOAD_TIMEOUT 5

// LORA MODULE SETTINGS (keep default unless experimental setup)

#define SERIAL_BAUD 9600                           // serial baud

#define LoRa_Frequency 433775E3                    // Frequency (433775000 Hz)
#define LoRa_TxPower 20                            // TX power (max 20 dBm)

#define LoRa_SCK 5
#define LoRa_MISO 19
#define LoRa_MOSI 27
#define LoRa_SS 18
#define LoRa_RST 14
#define LoRa_DIO0 26
#define LoRa_SpreadingFactor 12
#define LoRa_SignalBandwidth 125000
#define LoRa_CodingRate4 5

// DEBUG SETTINGS

#define ANEMO_DEBUG_MODE false                        // enable debug mode for hall sensor
#define HTTP_DEBUG_MODE false                          // print incoming HTTP requests
#define PM_DEBUG_MODE false                             // print PM values

// EXPERIMENTAL DEBUG ONLY SETTINGS
#define DIGI_IGNORE_PARADIGM false       // digipeat packets regardless if they contain WIDEn-N