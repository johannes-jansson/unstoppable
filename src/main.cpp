#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
// https://github.com/espressif/arduino-esp32/blob/master/libraries/HTTPClient/examples/BasicHttpsClient/BasicHttpsClient.ino
// https://randomnerdtutorials.com/esp32-https-requests/#esp32-https-requests-httpclient

#define GREEN_PIN 13
#define YELLOW_PIN 12
#define RED_PIN 14

unsigned long long uS_TO_S_FACTOR = 1000000;

const char* ssid     = "The Promised LAN";       
const char* password = ""; 

String weatherURL = "https://wttr.in/H%C3%A4ssleholm?format=j1";
String trafficURL = "https://www.skanetrafiken.se/gw-tps/api/v2/Journey?fromPointId=9021012093070000&fromPointType=STOP_AREA&toPointId=9021012080040000&toPointType=STOP_AREA&arrival=false&priority=SHORTEST_TIME&journeysAfter=1&walkSpeed=NORMAL&maxWalkDistance=2000&allowWalkToOtherStop=true";

int precipitation;
int deviation;

int parsePrecipitation(String jsonString) {
  // Go through every hour of current day, and if precipitation is ever greater
  // than zero, return 1. If not, return 0.
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, jsonString);
  if (err) {
    Serial.print("Deserialization failed!");
    Serial.println(err.f_str());
    return -1;
  }
  JsonArray hourly = doc["weather"][0]["hourly"];
  // Serial.print("Number of hours: ");
  // Serial.println(hourly.size());
  for (int i = 0; i < hourly.size(); i++) {
    if (hourly[i]["precipMM"] > 0) {
      return 1;
    }
  }
  return 0;
}

int parseTraffic(String jsonString) {
  // Go through every journey in the result, and if there are any deviations,
  // return 1. If not, return 0. If no journeys are found, return -1.
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, jsonString);
  if (err) {
    Serial.print("Deserialization failed!");
    Serial.println(err.f_str());
    return -1;
  }
  JsonArray deviations;
  JsonArray journeys = doc["journeys"];
  // Serial.print("Number of journeys: ");
  // Serial.println(journeys.size());
  if (journeys.size() < 1) {
    return -1;
  }
  for (int i = 0; i < journeys.size(); i++) {
    deviations = journeys[i]["deviationTags"];
    if (deviations.size() > 0) {
      return 1;
    }
  }
  return 0;
}

String httpsGETRequest(String endpoint) {
  String payload = "{}";
  WiFiClientSecure *client = new WiFiClientSecure;
  if (client) {
    client->setInsecure();
    HTTPClient https;

    Serial.println("[HTTPS] begin...");
    if (https.begin(*client, endpoint.c_str())) {
      https.addHeader("search-engine-environment", "TjP");
      https.addHeader("authority", "www.skanetrafiken.se");
      https.addHeader("accept", "application/json, text/plain, */*");
      https.addHeader("accept-language", "sv-SE");
      Serial.println("[HTTPS] GET...");
      int httpCode = https.GET();
      if (httpCode > 0) {
        Serial.println("[HTTPS] GET... code: "  + String(httpCode));
        payload = https.getString();
      } else {
        Serial.println("[HTTPS] GET... failed, error: " + https.errorToString(httpCode));
      }
      https.end();
    }
  } else {
    Serial.println("[HTTPS] Unable to connect");
  }
  // Serial.println(payload);
  return payload;
}

void setupWiFi() {
  WiFi.mode(WIFI_STA);             
  WiFi.begin(ssid, password);             
  Serial.print("Connecting to ");
  Serial.print(ssid);

  while (WiFi.status() != WL_CONNECTED) { 
    delay(500);
    Serial.print('.');
  }

  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
}

void update() {
  digitalWrite(GREEN_PIN,LOW);
  setupWiFi();
  String weatherPayload = httpsGETRequest(weatherURL);
  precipitation = parsePrecipitation(weatherPayload);
  String trafficPayload = httpsGETRequest(trafficURL);
  deviation = parseTraffic(trafficPayload);

  // Serial.print("Precipitation: ");
  // Serial.println(precipitation);
  // Serial.print("Deviations: ");
  // Serial.println(deviation);

  digitalWrite(GREEN_PIN,HIGH);
  if (precipitation > 0) {
    digitalWrite(YELLOW_PIN,HIGH);
  }
  if (deviation > 0) {
    digitalWrite(RED_PIN,HIGH);
  }
  if (precipitation < 0 || deviation < 0) {
    digitalWrite(GREEN_PIN,LOW);
  }
}


void setup() {
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
  digitalWrite(GREEN_PIN,LOW);
  digitalWrite(YELLOW_PIN,LOW);
  digitalWrite(RED_PIN,LOW);
  
  Serial.begin(115200);         
  delay(10);
  // delay(5000);
  Serial.println('\n');
}

void loop() {
  update();
  esp_sleep_enable_timer_wakeup(60 * uS_TO_S_FACTOR);
  Serial.flush();
  esp_light_sleep_start();
}




























// String getValue(HTTPClient &https, String key, int skip, int get) {
//   bool found = false, look = false;
//   int ind = 0;
//   String ret_str = "";
// 
//   int len = https.getSize();
//   char char_buff[1];
//   WiFiClient * stream = https.getStreamPtr();
//   while (https.connected() && (len > 0 || len == -1)) {
//     size_t size = stream->available();
//     if (size) {
//       int c = stream->readBytes(char_buff, ((size > sizeof(char_buff)) ? sizeof(char_buff) : size));
//       if (len > 0)
//         len -= c;
//       if (found) {
//         if (skip == 0) {
//           ret_str += char_buff[0];
//           get --;
//         } else
//           skip --;
//         if (get <= 0)
//           break;
//       }
//       else if ((!look) && (char_buff[0] == key[0])) {
//         look = true;
//         ind = 1;
//       } else if (look && (char_buff[0] == key[ind])) {
//         ind ++;
//         if (ind == key.length()) found = true;
//       } else if (look && (char_buff[0] != key[ind])) {
//         ind = 0;
//         look = false;
//       }
//     }
//   }
//   return ret_str;
// }
//
// String httpGETRequest(String endpoint) {
//   WiFiClient client;
//   HTTPClient http;
// 
//   http.begin(client, endpoint.c_str());
//   int httpResponseCode = http.GET();
// 
//   String payload = "{}";
//   if (httpResponseCode>0) {
//     Serial.print("HTTP Response code: ");
//     Serial.println(httpResponseCode);
//     payload = http.getString();
//   } else {
//     Serial.print("Error code: ");
//     Serial.println(httpResponseCode);
//   }
// 
//   http.end();
//   Serial.println(payload);
//   return payload;
// }
//
// String weatherURLOld = "https://api.open-meteo.com/v1/forecast?latitude=56.1591&longitude=13.7664&daily=precipitation_sum&timezone=Europe%2FBerlin&forecast_days=1";
// int parsePrecipitationOld(String jsonString) {
//   JsonDocument doc;
//   DeserializationError err = deserializeJson(doc, jsonString);
//   if (err) {
//     Serial.print("Deserialization failed!");
//     Serial.println(err.f_str());
//     return -1;
//   }
//   double p = doc["daily"]["precipitation_sum"][0];
//   Serial.print("Precipitation: ");
//   Serial.println(p);
//   if (p >= 5) {
//     return 1;
//   }
//   return 0;
// }
