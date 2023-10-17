#include <Arduino.h>
#include "config.h"
#include <LoRa.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_BMP280.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "website.h"

String VERSION = "4.2";
String DESTCALL_METEO = "APLDM0";
String DESTCALL_IGATE = "APLDI0";

void lora_setup();
void lora_send(String tx_data);
void aprsis_connect();
void upload_data(String upload_data);
void aprsis_send(String aprsis_packet);
void beacon_igate();
void beacon_meteo();
void beacon_meteo_status();
void beacon_upload();

bool check_wifi();
bool check_aprsis();

WiFiServer server(80);
WiFiClient aprsis;
HTTPClient upload;
String APRSISServer = APRS_IS_Server;
int pktIndex;
int mpktIndex;
int apktIndex;
AsyncWebServer serverWS(5028);
AsyncWebSocket ws("/ws");
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
void updateWebSocket();
String HTMLelementDef(String elementID);
String ipToString(IPAddress ip);
bool isWSconnected = false;
unsigned long lastWSupdate = 0;
String myIP;
bool GETIndex(String header, String requestPath);

unsigned long lastDigipeat = 0;
String lastRXstation = "no station";

float voltage = 0;
int battPercent = 0;

bool wifiStatus = false;
static time_t aprsLastReconnect = 0;
static time_t lastIgBeacon = 0;
static time_t lastMtBeacon = 0;
static time_t lastUpload = 0;

Adafruit_BMP280 bmp;
bool BMPstatus = false;
bool getBMPstatus();
float getBMPTempC();
String getBMPTempAPRS();
float getPressure();
String getPressureAPRS();
String tempToWeb(float tempValue);
String pressToWeb(int pressValue);
String windToWeb(float windValue);
String valueForJSON(String value);

String tempValues;
String pressValues;
String windValues;
float minTemp = -1000;
float maxTemp;
int minPress = -1000;
int maxPress;
float maxWind;
float maxGust;
String addGraphValue(String values, String value);
String generateGraph(String values, String graphName, String graphID, int r, int g, int b);

void hall_change();
float mph(float metersPerSeconds);
String windSpeedAPRS(float fSpeed);
int anemoACValue;
bool magnetDetected = false;
int windMeterSpins = 0;
int windMeterSpinsInTimeout = 0;
unsigned long windCycleDuration = 0;
unsigned long windTimeout = 0;
unsigned long windLastGust = 0;
float windActualSpeed = 0;
float windKMH(float windMS);
float windLongPeriodSpeed = 0;
float gust = 0;

bool meteoSwitch = USE_METEO;
bool aprsSwitch = Use_IGATE;

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println("\n\nLoRa Meteo v" + String(VERSION) + "\nby OK2DDS\n");
  lora_setup();
  delay(25);
  if (Use_WiFi) {
    WiFi.setAutoReconnect(true);
    WiFi.setHostname(Hostname);
    WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi..");
    wifiStatus = true;
    myIP = ipToString(WiFi.localIP());
    server.begin();
    ws.onEvent(onWsEvent);
    serverWS.addHandler(&ws);
    serverWS.begin();
    if (Use_IGATE && check_wifi()) {
      aprsis_connect();
    }
  }

  // BMP
  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 not connected");
    BMPstatus = false;
  } else {
    Serial.println("BMP280 OK");
    BMPstatus = true;
  }
  // HALL - digital
  if (USE_ANEMOMETER) pinMode(HALL_SENSOR_PIN, INPUT);
  //attachInterrupt(digitalPinToInterrupt(HALL_SENSOR_PIN), hall_change, HIGH);

  if (Use_IGATE) lastIgBeacon = millis() - int(IGATE_BEACON * 60000);
  delay(1000);
  voltage = float(analogRead(HALL_SENSOR_PIN)) / 4095*2*3.3*1.1;
  delay(10);
  beacon_meteo();
  Serial.println("Startup finished.");
}

void loop() {
  if (check_wifi() && wifiStatus == false) {
    wifiStatus = true;
    Serial.println("Wi-Fi connected");
  }
  if (Use_WiFi && !check_wifi() && wifiStatus) {
    wifiStatus = false;
    Serial.println("Wi-Fi not connected!");
    WiFi.reconnect();
  }
  if (Use_WiFi && check_wifi()) {
    if (aprsSwitch && Use_IGATE && !check_aprsis() && aprsLastReconnect + 60000 < millis()) {
      aprsis.stop();
      delay(100);
      aprsLastReconnect = millis();
      aprsis_connect();
    }
    if (myIP != ipToString(WiFi.localIP())) {
      myIP = ipToString(WiFi.localIP());
      Serial.println("IP: " + myIP);
    }
    WiFiClient client = server.available();
    if (client) {                             
    unsigned long currentTime = millis();
    unsigned long previousTime = currentTime;
    String header;
    Serial.println("HTTP from " + ipToString(client.remoteIP()) + ":" + String(client.remotePort()));     
    String currentLine = "";         
    while (client.connected() && currentTime - previousTime <= 5000) {
      currentTime = millis();
      if (client.available()) { 
        char c = client.read();
        header += c;
        if (c == '\n') {
          if (currentLine.length() == 0) {
            if (HTTP_DEBUG_MODE) Serial.println(header);
            client.println("HTTP/1.1 200 OK");
            if (GETIndex(header, "/api/meteo"))
              client.println("Content-type:text/csv");
            else if (header.indexOf("json") >= 0)
              client.println("Content-type:application/json");
            else
              client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            if (header.indexOf("favicon.ico") == -1) {
            if (GETIndex(header, "/api")) {
              // API responses without user frontend layout
              if (GETIndex(header, "/api/meteo"))
                client.println(tempToWeb(getBMPTempC()) + "," + pressToWeb(int(getPressure())) + "," + windToWeb(windActualSpeed) + "," + windToWeb(windLongPeriodSpeed) + "," + windToWeb(gust));
              if (GETIndex(header, "/api/graphs-json"))
                client.println("{\"temperature\": [" + String(tempValues) + "], \"pressure\": [" + String(pressValues) + "], \"wind\": [" + String(windValues) + "]}");
              if (GETIndex(header, "/api/json"))
                client.println("{\"general\": {\"version\":\"" + String(VERSION) + "\", \"destcall_meteo\":\"" + String(DESTCALL_METEO) + "\", \"destcall_igate\":\"" + String(DESTCALL_IGATE) + "\", \"system_time\":" + String(millis()) + ", \"voltage\":" + String(voltage) + ", \"battery\":" + String(battPercent) + ", \"wifi_status\":" + (check_wifi() ? "true" : "false") + ", \"wifi_signal_db\":" + (check_wifi() ? String(WiFi.RSSI()) : "0") + ", \"wifi_ssid\":\"" + String(WiFi.SSID()) + "\", \"wifi_hostname\":\"" + String(Hostname) + "\", \"bmp280_status\":" + (getBMPstatus() ? "true" : "false") + "}, \"lora\": {\"meteo_callsign\":\"" + String(METEO_CALLSIGN) + "\", \"meteo_enabled\":" + (meteoSwitch ? "true" : "false") + ", \"igate_callsign\":\"" + String(IGATE_CALLSIGN) + "\", \"aprs_is_enabled\":" + (aprsSwitch ? "true" : "false") + ", \"aprs_is_status\":" + (check_aprsis() ? "true" : "false") + ", \"aprs_is_server\":\"" + (check_aprsis() ? String(APRSISServer) : "disconnected") + "\", \"hall_sensor\":" + String(anemoACValue) + ", \"last_rx\":\"" + String(lastRXstation) + "\"" + "}, \"meteo\": {\"temperature\":" + valueForJSON(tempToWeb(getBMPTempC())) + ", \"pressure\":" + valueForJSON(pressToWeb(int(getPressure()))) + ", \"actual_wind\":" + valueForJSON(windToWeb(windActualSpeed)) + ", \"long_period_wind\":" + valueForJSON(windToWeb(windLongPeriodSpeed)) + ", \"gust\":" + valueForJSON(windToWeb(gust)) + ", \"min_temperature\":" + (getBMPstatus() ? String(minTemp) : "0") + ", \"max_temperature\":" + (getBMPstatus() ? String(maxTemp) : "0") + ", \"min_pressure\":" + (getBMPstatus() ? String(minPress) : "0") + ", \"max_pressure\":" + (getBMPstatus() ? String(maxPress) : "0") + ", \"max_wind\":" + String(maxWind) + ", \"max_gust\":" + String(maxGust) + "}}");
            } else {
            client.println(String(webPageStart));
            if (!GETIndex(header, "/watch")) client.println(String(webPageHeader) + "</h1><br>");
            if (GETIndex(header, "/switch-meteo")) {
              meteoSwitch = !meteoSwitch;
              client.println(webReload);
            }
            if (GETIndex(header, "/switch-aprs")) {
              aprsSwitch = !aprsSwitch;
              client.println(webReload);
              aprsis.stop();
              delay(100);
            }
            if (GETIndex(header, "/restart")) {
              client.println();
              ESP.restart();
            }
            if (GETIndex(header, "/reset-tx")) {
              lastUpload = millis();
              lastIgBeacon = millis();
              lastMtBeacon = millis();
              client.println("<br>TX reset done.<br>");
            }
            if (GETIndex(header, "/reset-temp")) {
              minTemp = getBMPTempC();
              maxTemp = minTemp;
              client.println("<br>Temperature reset done.<br>");
            }
            if (GETIndex(header, "/reset-press")) {
              minPress = getPressure();
              maxPress = minPress;
              client.println("<br>Pressure reset done.<br>");
            }
            if (GETIndex(header, "/reset-bmp")) {
              minTemp = getBMPTempC();
              maxTemp = minTemp;
              minPress = getPressure();
              maxPress = minPress;
              client.println("<br>BMP values reset done.<br>");
            }
            if (GETIndex(header, "/reset-wind")) {
              maxWind = windLongPeriodSpeed;
              maxGust = windActualSpeed;
              client.println("<br>Wind reset done.<br>");
            }
            if (GETIndex(header, "/lora"))
              client.println("<br>Version: " + String(VERSION) + "<br><br> Voltage: " + String(voltage) + "V<br>Battery: " + String(battPercent) + "%<br>Wi-Fi: " + (check_wifi() ? String(WiFi.SSID()) + " " + String(WiFi.RSSI()) + " dB<br>IP: " + ipToString(WiFi.localIP()) : String("not connected")) + String("<br>APRS-IS: ") + (aprsis.connected() ? "connected" : "not connected") + "<br>Last RX: " + String(lastRXstation) + "<br>Hall sensor: " + String(anemoACValue) + "<br><br>");
            if ((GETIndex(header, "/lora") || GETIndex(header, "/min-max")) && USE_METEO && (BMPstatus || USE_ANEMOMETER)) {
              client.println("<table><tr><td></td><td><b>Minimum</b></td><td><b>Maximum</b></td></tr>");
              if (BMPstatus) {
                client.println("<tr><td><b>Temperature</b></td><td>" + String(minTemp) + " &deg;C</td><td>" + String(maxTemp) + " &deg;C</td></tr>");
                client.println("<tr><td><b>Pressure</b></td><td>" + String(minPress) + " hPa</td><td>" + String(maxPress) + " hPa</td></tr>");
              }
              if (USE_ANEMOMETER) {
                client.println("<tr><td><b>Avg. wind</b></td><td>0.00 m/s</td><td>" + String(maxWind) + " m/s</td></tr>");
                client.println("<tr><td><b>Wind gust</b></td><td>0.00 m/s</td><td>" + String(maxGust) + " m/s</td></tr>");
              }
              client.println("</table><br>");
            }
            if (GETIndex(header, "/lora")) {
              client.println("<br>Reset values<br><a href='/reset-bmp'>BMP values</a> - <a href='/reset-temp'>Temperature</a> - <a href='/reset-press'>Pressure</a> - <a href='/reset-wind'>Wind</a><br>");
              client.println("<br><a href='/switch-meteo'>Turn meteo On/Off</a> (" + String(meteoSwitch ? "ON" : "OFF") + ")");
              client.println("<br><a href='/switch-aprs'>Turn IGate On/Off</a> (" + String(aprsSwitch ? "ON" : "OFF") + ")");
              client.println("<br><a href='/change-aprsis'>Change APRS-IS server</a> (" + String(APRSISServer) + ")");
              client.println("<br><a href='/restart'>Restart device</a>");
              client.println("<br><br><a href='/'>View main meteo page</a>");
            }
            if (GETIndex(header, "/graphs")) {
              if (BMPstatus || USE_ANEMOMETER)
                client.println("<script src='https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.9.4/Chart.js'></script>");
              else
                client.println("<br>No graphs to display.<br>");
              if (BMPstatus) {
                client.println(generateGraph(tempValues, "Temperature", "temp", 230, 0, 0));
                client.println(generateGraph(pressValues, "Pressure", "press", 0, 125, 0));
              }
              if (USE_ANEMOMETER)
                client.println(generateGraph(windValues, "Average wind (m/s)", "wind", 0, 0, 255));
              client.println("<a href='/'>View main meteo page</a>");
            }
            if (GETIndex(header, "/tx")) {
              pktIndex = header.indexOf("/tx/");
              client.println("OK");
              Serial.println("Custom packet via TCP");
              String custPkt;
              custPkt = header.substring(pktIndex + 4, header.indexOf("HTTP/") - 1);
              while (custPkt.indexOf("%3E") != -1) {
                custPkt = custPkt.substring(0, custPkt.indexOf("%3E")) + ">" + custPkt.substring(custPkt.indexOf("%3E") + 3);
              }
              while (custPkt.indexOf("%20") != -1) {
                custPkt = custPkt.substring(0, custPkt.indexOf("%20")) + " " + custPkt.substring(custPkt.indexOf("%20") + 3);
              }
              while (custPkt.indexOf("%3A") != -1) {
                custPkt = custPkt.substring(0, custPkt.indexOf("%3A")) + ":" + custPkt.substring(custPkt.indexOf("%3A") + 3);
              }
              while (custPkt.indexOf("%2F") != -1) {
                custPkt = custPkt.substring(0, custPkt.indexOf("%2F")) + "/" + custPkt.substring(custPkt.indexOf("%2F") + 3);
              }
              while (custPkt.indexOf("%40") != -1) {
                custPkt = custPkt.substring(0, custPkt.indexOf("%40")) + "@" + custPkt.substring(custPkt.indexOf("%40") + 3);
              }
              while (custPkt.indexOf("%7B") != -1) {
                custPkt = custPkt.substring(0, custPkt.indexOf("%7B")) + "{" + custPkt.substring(custPkt.indexOf("%7B") + 3);
              }
              lora_send(custPkt);
              custPkt = "";
              pktIndex = 0;
              delay(3000);
            }
            if (GETIndex(header, "/change-aprsis")) {
              client.println(webAPRSISChangePrompt);
            }
            if (GETIndex(header, "/new-aprsis")) {
              apktIndex = header.indexOf("GET /new-aprsis");
              String newServer = header.substring(apktIndex + 16, header.indexOf("HTTP/") - 1);
              Serial.println("New APRS-IS server request: " + String(newServer));
              if (newServer != "null" && Use_WiFi && Use_IGATE && check_wifi()) {
                String backupServer = APRSISServer;
                APRSISServer = newServer;
                aprsis.stop();
                delay(100);
                aprsis_connect();
                delay(100);
                if (check_aprsis() && check_wifi()) {
                  client.println(webAPRSISChangeSuccess);
                  Serial.println("Success. New server: " + String(APRSISServer));
                } else {
                  client.println(webAPRSISChangeError);
                  Serial.println("Connection not successful.");
                  APRSISServer = backupServer;
                  aprsis.stop();
                  delay(100);
                  if(aprsSwitch) aprsis_connect();
                }
              } else {
                client.println(webAPRSISChangeError);
                Serial.println("Bad input. Change not successful.");
              }
              apktIndex = 0;
            }
            if (GETIndex(header, "/ ")) {
              // ORDINARY METEO WEBSITE
              client.println(webMeteoOnlineIndicator);
              client.println(webMeteoLayout);
              client.println(webSocketSetupScript);
              client.println(HTMLelementDef("onlineIndicator") + HTMLelementDef("temp") + HTMLelementDef("press") + HTMLelementDef("wind") + HTMLelementDef("windkmh") + HTMLelementDef("gust") + HTMLelementDef("windlp"));
              client.println(webMeteoOnlineRoutine);
              client.println(webSocketHandleScript);
            }
            if (GETIndex(header, "/watch")) {
              // METEO WEBSITE LAYOUT FOR WATCH
              client.println(webMeteoWatchLayout);
              client.println("<script>var dat = '" + tempToWeb(getBMPTempC()) + "," + pressToWeb(int(getPressure())) + "," + windToWeb(windActualSpeed) + "," + windToWeb(windKMH(windActualSpeed)) + "'; ");
              client.println(HTMLelementDef("temp") + HTMLelementDef("press") + HTMLelementDef("wind") + HTMLelementDef("windkmh"));
              client.println(webWatchValuesScript);
            }
            if (!GETIndex(header, "/watch"))
              client.println(webPageFooter);
            client.println(webPageEnd);
            }
            }
            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();
    }
  }

  ws.cleanupClients();
  if (Use_WiFi && isWSconnected && lastWSupdate + 700 < millis()) updateWebSocket();

  if (Use_IGATE && check_wifi() && check_aprsis() && lastIgBeacon + (IGATE_BEACON * 60000) < millis()) beacon_igate();
  if (meteoSwitch && lastMtBeacon + (METEO_BEACON * 60000) < millis()) beacon_meteo();
  if (Use_UPLOAD && check_wifi() && lastUpload + (UPLOAD_TIMEOUT * 60000) < millis()) beacon_upload();

  voltage = float(analogRead(HALL_SENSOR_PIN)) / 4095*2*3.3*1.1;
  battPercent = 100 * (voltage - 3.3) / (4.2 - 3.3);
  if (battPercent > 100) battPercent = 100;
  if (battPercent < 0) battPercent = 0;

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
  while (LoRa.available()) {
    bool digiOutput = false;
    String destCall, digiPath, originalPath, sourceCall, message, digiPacket, statusMessage;
    int pos1, pos2;
    String rxPacket = LoRa.readString();
    rxPacket = rxPacket.substring(3);
    Serial.println("RX: " + rxPacket);

    if (!(rxPacket.length() < 5 || rxPacket.indexOf('>') < 5 || rxPacket.indexOf(':') < rxPacket.indexOf('>') || rxPacket.substring(rxPacket.indexOf('>') + 1, rxPacket.indexOf(':')) == "") && Use_IGATE && Use_WiFi && aprsSwitch) {
      String igatePacket = rxPacket;
      if (igatePacket.indexOf("NOGATE") == -1 && igatePacket.indexOf("RFONLY") == -1 && igatePacket.indexOf("TCPIP") == -1 && igatePacket.indexOf("TCPXX") == -1 && igatePacket.indexOf(String(METEO_CALLSIGN)) == -1 && igatePacket.indexOf(String(IGATE_CALLSIGN) + "*") == -1 && rxPacket.substring(0, rxPacket.indexOf('>')) != String(IGATE_CALLSIGN)) {
        igatePacket = igatePacket.substring(0, igatePacket.indexOf(":")) + ",qAO," + String(IGATE_CALLSIGN) + igatePacket.substring(igatePacket.indexOf(":"));
        //Serial.println("IGated packet.");
        aprsis_send(igatePacket);
      }
    }

      // digipeating
      pos1 = rxPacket.indexOf('>');
      if (pos1 < 3)
        goto bad_packet;
      sourceCall = rxPacket.substring(0, pos1);
      pos2 = rxPacket.indexOf(':');
      if (pos2 < pos1)
        goto bad_packet;
      destCall = rxPacket.substring(pos1 + 1, pos2);
      message = rxPacket.substring(pos2 + 1);
      digiPath = "";
      pos2 = destCall.indexOf(',');
      if (pos2 > 0) {
        digiPath = destCall.substring(pos2 + 1);
        destCall = destCall.substring(0, pos2);
      }
      originalPath = digiPath;
      if (destCall == "")
        goto bad_packet;
      lastRXstation = sourceCall;
      if (int callIndex = digiPath.indexOf(String(IGATE_CALLSIGN)) > -1 && digiPath.indexOf(String(IGATE_CALLSIGN) + "*") == -1) {
        digiPath = digiPath.substring(0, callIndex - 1) + digiPath.substring(callIndex + String(IGATE_CALLSIGN).length());
      }
      if (int paradigmIndex = digiPath.indexOf("WIDE1-") > -1 && USE_DIGIPEATER && digiPath.indexOf(String(IGATE_CALLSIGN) + "*") == -1 && rxPacket.indexOf(String(METEO_CALLSIGN)) == -1 && sourceCall.indexOf(String(IGATE_CALLSIGN)) == -1) {
        paradigmIndex = digiPath.indexOf("WIDE1-");
        if (paradigmIndex == 0)
          paradigmIndex = 1;
        //Serial.println(digiPath.substring(0, paradigmIndex - 1));
        if (paradigmIndex == 1)
          digiPath = digiPath.substring(0, paradigmIndex - 1) + "," + String(IGATE_CALLSIGN) + "*,WIDE1*" + digiPath.substring(paradigmIndex + 6);
        else
          digiPath = digiPath.substring(0, paradigmIndex - 1) + "," + String(IGATE_CALLSIGN) + "*,WIDE1*" + digiPath.substring(paradigmIndex + 7);
        // add SNR and RSSI
        //message = message + " SNR=" + String(LoRa.packetSnr()) + "dB RSSI=" + String(LoRa.packetRssi()) + "dB";
        if (digiPath.indexOf(",") != 0)
          digiPath = "," + digiPath;
        digiPacket = sourceCall + ">" + destCall + digiPath + ":" + message;
        lora_send(digiPacket);
      } else if (USE_DIGIPEATER && DIGI_IGNORE_PARADIGM && digiPath.indexOf("*") == -1 && (millis() > lastDigipeat + 600000 || lastDigipeat == 0) && digiPath.indexOf(String(IGATE_CALLSIGN) + "*") == -1 && rxPacket.indexOf(String(METEO_CALLSIGN)) == -1 && sourceCall.indexOf(String(IGATE_CALLSIGN)) == -1) {
        lastDigipeat = millis();
        digiPath = digiPath + "," + String(IGATE_CALLSIGN) + "*";
        if (digiPath.indexOf(",") != 0)
          digiPath = "," + digiPath;
        // do not add SNR and RSSI
        digiPacket = sourceCall + ">" + destCall + digiPath + ":" + message;
        lora_send(digiPacket);
	    // do not digipeat without WIDE1-1
      } else if (USE_DIGIPEATER && DIGI_IGNORE_PARADIGM) {
        Serial.println("Station not repeated.");
      }
      digiOutput = true;

      // send status
      statusMessage = String(IGATE_CALLSIGN) + ">" + String(DESTCALL_IGATE) + ":>Last RX: " + String(sourceCall) + " SNR=" + String(LoRa.packetSnr()) + "dB RSSI=" + String(LoRa.packetRssi()) + "dB";
      if (aprsSwitch && USE_LASTRX_STATUS && originalPath.indexOf("*") == -1)
        aprsis_send(statusMessage);

    bad_packet:
      if (!digiOutput) Serial.println("Bad packet");
  }
  }

  while (Use_IGATE && check_aprsis() && aprsis.available()) {
    String apstring;
    char aprx = aprsis.read();
    apstring += aprx;
    if (aprx == '\n') {
      //Serial.println(apstring);
      if (apstring.indexOf("logresp") == -1) {
        // incoming packet handling
      }
      apstring = "";
    }
  }

  if (USE_ANEMOMETER) {
  anemoACValue = analogRead(HALL_SENSOR_PIN);
  if (ANEMO_DEBUG_MODE) Serial.println(anemoACValue);
  if (anemoACValue <= ANEMO_AC_THRESHOLD && !magnetDetected) {
    magnetDetected = true;
    hall_change();
  } else if (anemoACValue >= ANEMO_AC_LOSE) {
    magnetDetected = false;
  }
  if (windMeterSpins >= int(ANEMO_RECALC_LIMIT)) {
    windActualSpeed = float(1 / ((millis() - windCycleDuration) / (windMeterSpins * ANEMOMETER_LENGTH * 1000)));
    //windActualSpeed = float(windMeterSpins * 1000 / (millis() - windCycleDuration)) * float(ANEMOMETER_LENGTH);
    windCycleDuration = millis();
    Serial.println("Wind: " + String(windActualSpeed) + " m/s");
    windMeterSpins = 0;
    if (windActualSpeed > gust) {
      gust = windActualSpeed;
      windLastGust = millis();
    }
  }
  if (windTimeout + (ANEMO_RECALC_LIMIT_TIMEOUT * 1000) < millis()) {
    windLongPeriodSpeed = float(1 / ((millis() - windTimeout) / (windMeterSpinsInTimeout * ANEMOMETER_LENGTH * 1000)));
    //windLongPeriodSpeed = float(windMeterSpinsInTimeout * 1000 / (millis() - windTimeout)) * float(ANEMOMETER_LENGTH);;
    windTimeout = millis();
    Serial.println("Long-period wind: " + String(windLongPeriodSpeed) + " m/s");
    windMeterSpinsInTimeout = 0;
  }
  if (millis() > windCycleDuration + (ANEMO_RECALC_ACTUAL_SPEED * 1000)) windActualSpeed = 0;
  if (millis() > windLastGust + (ANEMO_RECALC_LIMIT_TIMEOUT * 1000)) gust = windActualSpeed;
  }
}

void lora_setup() {
  SPI.begin(LoRa_SCK, LoRa_MISO, LoRa_MOSI, LoRa_SS);
  LoRa.setPins(LoRa_SS, LoRa_RST, LoRa_DIO0);
  if (!LoRa.begin(LoRa_Frequency)) {
    Serial.println("Failed to setup LoRa module.");
    while (1);
  }
  LoRa.setSpreadingFactor(LoRa_SpreadingFactor);
  LoRa.setSignalBandwidth(LoRa_SignalBandwidth);
  LoRa.setCodingRate4(LoRa_CodingRate4);
  LoRa.enableCrc();
  LoRa.setTxPower(LoRa_TxPower);
  delay(3000);
  if (!Use_IGATE) {
    LoRa.sleep();
  }
}

void lora_send(String tx_data) {
  LoRa.setFrequency(LoRa_Frequency);
  LoRa.beginPacket();
  LoRa.write('<');
  LoRa.write(0xFF);
  LoRa.write(0x01);
  Serial.println("TX: " + tx_data);
  LoRa.write((const uint8_t *)tx_data.c_str(), tx_data.length());
  LoRa.endPacket();
  LoRa.setFrequency(LoRa_Frequency);
  if (!Use_IGATE) {
    LoRa.sleep();
  }
}

void aprsis_connect() {
  if (Use_WiFi && Use_IGATE) {
    if (aprsis.connect(APRSISServer.c_str(), APRS_IS_Port)) {
      Serial.println("APRS-IS OK");
    } else {
      Serial.println("APRS-IS error");
    }
    if (check_wifi() && check_aprsis()) {
      aprsis.println("user " + String(IGATE_CALLSIGN) + " pass " + String(APRS_IS_Password) + " vers LoRa_APRS_Meteo " + String(VERSION));
      aprsSwitch = true;
    }
  }
}

void upload_data(String upload_data) {
  Serial.println("HTTP: " + String(upload_data));
  if (check_wifi()) {
  String path = String(SERVER_URL) + upload_data;
  upload.begin(path.c_str());
  int response = upload.GET();
  if (response > 0) {
    Serial.println("HTTP: " + String(response));
  } else {
    Serial.println("HTTP ERROR: " + String(response));
  }
  }
}

void aprsis_send(String aprsis_packet) {
  if (!check_aprsis() && aprsSwitch && Use_IGATE && check_wifi()) {
    aprsis.stop();
    delay(100);
    aprsis_connect();
  } else if (check_wifi() && check_aprsis()) {
    Serial.println("APRS-IS: " + String(aprsis_packet));
    aprsis.println(aprsis_packet);
  } else {
    Serial.println("APRS-IS TX error");
  }
}

void beacon_igate() {
  lastIgBeacon = millis();
  if (Use_IGATE && check_wifi()) {
    String beacon = String(IGATE_CALLSIGN) + ">" + String(DESTCALL_IGATE) + ":!" + String(IGATE_LAT) + "L" + String(IGATE_LON) + "&" + String(IGATE_COMMENT);
    if (IGATE_BCN_NETWORK) {
      aprsis_send(beacon);
    } else if (Use_IGATE) {
      lora_send(beacon);
    }
    if (check_wifi() && USE_METEO_STATUS) beacon_meteo_status();
  }
}

void beacon_meteo() {
  lastMtBeacon = millis();
  if (meteoSwitch && BMPstatus) {
    String meteoBeacon = String(METEO_CALLSIGN) + ">" + String(DESTCALL_METEO) + ":!" + String(METEO_LAT) + "/" + String(METEO_LON) + "_.../" + String(windSpeedAPRS(windLongPeriodSpeed)) + "g" +  String(windSpeedAPRS(gust)) + "t" + String(getBMPTempAPRS()) + "b" + String(getPressureAPRS()) + String(METEO_COMMENT) + " U=" + String(voltage) + "V";
    lora_send(meteoBeacon);
  }
  if (BMPstatus) {
    float temp = getBMPTempC();
    int press = int(getPressure());
    String stemp = String(temp);
    String spress = String(press);
    tempValues = addGraphValue(tempValues, stemp);
    if (temp < minTemp || minTemp == -1000) minTemp = temp;
    if (temp > maxTemp) maxTemp = temp;
    pressValues = addGraphValue(pressValues, spress);
    if (press < minPress || minPress == -1000) minPress = press;
    if (press > maxPress) maxPress = press;
  } else {
    tempValues = addGraphValue(tempValues, "N/A");
    pressValues = addGraphValue(pressValues, "N/A");
  }
  if (USE_ANEMOMETER) {
    windValues = addGraphValue(windValues, String(windLongPeriodSpeed));
    if (windLongPeriodSpeed > maxWind) maxWind = windLongPeriodSpeed;
    if (gust > maxGust) maxGust = gust;
  } else {
    windValues = addGraphValue(windValues, "N/A");
  }
  gust = 0;
}

void beacon_meteo_status() {
  String meteoStatus = String(METEO_CALLSIGN) + ">" + String(DESTCALL_METEO) + ":>" + String(METEO_STATUS);
  aprsis_send(meteoStatus);
}

void beacon_upload() {
  lastUpload = millis();
  if (Use_UPLOAD) upload_data(String(getBMPTempC()) + "," + String(int(getPressure())) + "," + String(windActualSpeed) + "," + String(windLongPeriodSpeed) + "," + String(voltage) + "," + String(gust));
}

bool check_wifi() {
  if (WiFi.status() == WL_CONNECTED)
    return true;
  else
    return false;
}

bool check_aprsis() {
  if (aprsis.connected() && check_wifi())
    return true;
  else
    return false;
}

float getBMPTempC() {
  if (BMPstatus)
    return bmp.readTemperature() + BMP_OFFSET_TEMP;
  else return 0;
}

String getBMPTempAPRS() {
  if (BMPstatus) {
  float fahrenheit = getBMPTempC();
  fahrenheit *= 9;
  fahrenheit /= 5;
  int ifahrenheit = fahrenheit + 32;
  if (!(ifahrenheit < 1000)) ifahrenheit = 0;
  String sfahrenheit = String(ifahrenheit);
  if (ifahrenheit < 100) sfahrenheit = String("0") + String(sfahrenheit);
  if (ifahrenheit < 10) sfahrenheit = String("0") + String(sfahrenheit);
  return sfahrenheit;
  } else return "000";
}

float getPressure() {
  if (BMPstatus)
    return (bmp.readPressure() / 100) + BMP_OFFSET_PRESS;
  else
    return 0;
}

String getPressureAPRS() {
  if (BMPstatus) {
  int press = getPressure();
  press *= 10;
  if (press > 99999) press = 0;
  String spress = String(press);
  if (press < 10000) spress = String("0") + String(spress);
  if (press < 1000) spress = String("0") + String(spress);
  if (press < 100) spress = String("0") + String(spress);
  if (press < 10) spress = String("0") + String(spress);
  return spress;
  } else {
    return "00000";
  }
}

void hall_change() {
  windMeterSpins++;
  windMeterSpinsInTimeout++;
}

float mph(float metersPerSeconds) {
  return metersPerSeconds * 2.23693629;
}

String windSpeedAPRS(float fSpeed) {
  if (USE_ANEMOMETER) {
    int wSpeed = int(mph(fSpeed));
    if (wSpeed >= 1000) wSpeed = 0;
    String sSpeed = String(wSpeed);
    if (wSpeed < 100) sSpeed = "0" + String(sSpeed);
    if (wSpeed < 10) sSpeed = "0" + String(sSpeed);
    return sSpeed;
  } else {
    return "...";
  }
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    Serial.println("WS Con: " + String(ws.count()) + " connected.");
    isWSconnected = true;
  } else if(type == WS_EVT_DISCONNECT){
    Serial.println("WS Dis: " + String(ws.count()) + " connected.");
    if(ws.count() == 0) isWSconnected = false;
  }
}

void updateWebSocket() {
  ws.textAll(tempToWeb(getBMPTempC()) + "," + pressToWeb(int(getPressure())) + "," + windToWeb(windActualSpeed) + "," + windToWeb(windKMH(windActualSpeed)) + "," + windToWeb(gust) + "," + windToWeb(windLongPeriodSpeed));
  lastWSupdate = millis();
  //Serial.println("WS: Updated.");
}

String HTMLelementDef(String elementID) {
  return " var " + elementID + " = document.getElementById('" + elementID + "'); ";
}

String ipToString(IPAddress ip){
  String s="";
  for (int i=0; i<4; i++)
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  return s;
}

float windKMH(float windMS) {
  return windMS * 3.6;
}

String tempToWeb(float tempValue) {
  if (BMPstatus) return String(tempValue);
  else return "N/A";
}

String pressToWeb(int pressValue) {
  if (BMPstatus) return String(pressValue);
  else return "N/A";
}

String windToWeb(float windValue) {
  if (USE_ANEMOMETER) return String(windValue);
  else return "N/A";
}

String valueForJSON(String value) {
  if (value == "N/A")
    return "null";
  else
    return value;
}

String addGraphValue(String values, String value) {
  int count = 0;
  char searchChar = ',';
  for (int i = 0; i < values.length(); i++) {
    if (values[i] == searchChar) {
      count++;
    }
  }
  if (count > int(GRAPH_LIMIT) - 2)
    values = values.substring(values.indexOf(",") + 1);
  if (values != "") values += ",";
  values += valueForJSON(value);
  return values;
}

String generateGraph(String values, String graphName, String graphID, int r, int g, int b) {
  String graphScript = "<b style='width: 100%; text-align: center'>" + graphName +"</b><br><br><canvas id='" + graphID + "' style='width:100%'></canvas> \
  <script>var yValues = [" + values + "]; \
  var xValues = [";

  int count = 0;
  char searchChar = ',';
  for (int i = 0; i < values.length(); i++) {
    if (values[i] == searchChar) {
      count++;
    }
  }
  for (int i = 0; i <= count; i++) {
    graphScript += "' '";
    if (i != count) graphScript += ",";
  }
  
  graphScript += "];";
  if (values != "") {
    graphScript += "var " + graphID + "_min = Math.min(...yValues); var " + graphID + "_max = Math.max(...yValues);";
  } else {
    graphScript += "var " + graphID + "_min = 0; var " + graphID + "_max = 0;";
  }
  graphScript += "new Chart('" + graphID + "', {\
  type: 'line',\
  data: {\
    labels: xValues,\
    datasets: [{\
      fill: false,\
      lineTension: 0,\
      backgroundColor: 'rgba(";
  graphScript += String(r) + "," + String(g) + "," + String(b);
  graphScript += ",1.0)',";
  graphScript += "borderColor: 'rgba(";
  graphScript += String(r) + "," + String(g) + "," + String(b);
  graphScript += ",0.1)',\
      data: yValues\
    }]\
  },\
  options: {\
    legend: {display: false},\
    scales: {\
      yAxes: [{ticks: {min: " + graphID + "_min, max: " + graphID + "_max}}]\
    }\
  }\
  });\
  </script><br>";
  return graphScript;
}

bool GETIndex(String header, String requestPath) {
  if (header.indexOf("GET " + requestPath) >= 0)
    return true;
  else
    return false;
}

bool getBMPstatus() {
  return BMPstatus;
}
