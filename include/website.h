#define webPageStart "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><meta charset=\"utf-8\"><style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;} body { height: 99%;} footer { color: #696969; width: 100%; text-align: center;} a { color: #696969; font-family: Helvetica;} a:visited { color: #696969;} a:hover { color: #002fba;} canvas { margin: 0 auto;} .header { font-size: 14pt; text-align: right; width: 50%; padding-right: 25px;} .value { font-weight: bold; font-size: 14pt; text-align: left; width: 50%; padding-left: 25px;} sup { vertical-align: top; font-size: 0.6em;} b { padding: 7px; border-radius: 5px;} .cold { color: #004594;} .hot { color: #940000;} .good { color: #00702f;} .fair { color: #5b9400;} .moderate { color: #736f00;} .hazardous { color: #400094;} #meteoArrowWrapper { display: flex; flex-wrap: wrap; align-items: center; padding-bottom: 40px;} #weatherSymbol { font-size: 56pt;} #temp { font-size: 26pt;} .flex { flex-grow: 1; flex-basis: 50%; padding: 0; text-align: center;}</style><title>LoRa Meteo</title></head><body>"
#define webPageFooter "<br><br><footer><b><a href=\"/charts\">Weather charts &#128202;</a></b><br><a href=\"/lora\">View dashboard</a><br><br>Created by <a href=\"https://www.ok2dds.cz\" target=\"_blank\">OK2DDS</a>.</footer>"
#define webPageEnd "</body></html>"
#define webReload "<script>window.location.href = '/lora';</script>"
#define webAPRSISChangePrompt "<script>window.location.href = '/new-aprsis/' + prompt('Enter new APRS-IS server address', null);</script>"
#define webAPRSISChangeError "<script>alert('Error setting new APRS-IS server'); window.location.href = '/lora';</script>"
#define webAPRSISChangeSuccess "<script>alert('Connected to new APRS-IS server'); setTimeout(window.location.href = '/lora', 4500);</script>"

#define webSocketSetupScript "<script>var ip = location.host.split(':')[0]; const ws = new WebSocket('ws://' + ip + ':5028/ws'); ws.onopen = function() { console.log('WS: Connection opened');}; ws.onclose = function() { console.log('WS: Connection closed'); };"
#define webSocketHandleScriptDef "ws.onmessage = function(event) { var dat = event.data; var bmp = true; var anemo = true; var sps = true; if (dat.split(',')[0] == 'N/A' && dat.split(',')[1] == 'N/A') { bmp = false;} if (dat.split(',')[2] == 'N/A' && dat.split(',')[3] == 'N/A' && dat.split(',')[4] == 'N/A' && dat.split(',')[5] == 'N/A') { anemo = false;} if (dat.split(',')[6] == 'N/A' && dat.split(',')[7] == 'N/A') { sps = false;}"
#define webSocketHandleScriptElements "if (bmp) { temp.innerHTML = dat.split(',')[0] + ' &deg;C'; press.innerHTML = dat.split(',')[1] + ' hPa'; ptrend.innerHTML = dat.split(',')[9];} if (anemo) { wind.innerHTML = dat.split(',')[2] + ' m/s'; windkmh.innerHTML = dat.split(',')[3] + ' km/h'; windkn.innerHTML = dat.split(',')[4] + ' kn'; gust.innerHTML = dat.split(',')[5] + ' m/s'; windlp.innerHTML = dat.split(',')[6] + ' m/s'; pm25.innerHTML = dat.split(',')[7] + ' &mu;g/m<sup>3</sup>'; pm10.innerHTML = dat.split(',')[8] + ' &mu;g/m<sup>3</sup>';}"
#define webSocketHandleScriptColors "if (bmp && dat.split(',')[0] > 27) { temp.className = 'value flex hot';} else if (bmp && dat.split(',')[0] < 15) { temp.className = 'value flex cold';} else { temp.className = 'value flex';} if (sps) { var val25 = dat.split(',')[6]; switch (true) { case (val25 < 10): pm25.className = 'value good'; break; case (val25 < 20): pm25.className = 'value fair'; break; case (val25 < 25): pm25.className = 'value moderate'; break; case (val25 < 50): pm25.className = 'value hot'; break; case (val25 >= 50): pm25.className = 'value hazardous'; break; } var val10 = dat.split(',')[7]; switch (true) { case (val10 < 20): pm10.className = 'value good'; break; case (val10 < 35): pm10.className = 'value fair'; break; case (val10 < 50): pm10.className = 'value moderate'; break; case (val10 < 100): pm10.className = 'value hot'; break; case (val10 >= 100): pm10.className = 'value hazardous'; break; }} lastData = Date.now(); };</script>"