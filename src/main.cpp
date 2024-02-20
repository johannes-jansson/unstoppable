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

const char* ssid     = "The Promised LAN";       
const char* password = ""; 
String weatherURL = "https://api.open-meteo.com/v1/forecast?latitude=56.1591&longitude=13.7664&daily=precipitation_sum&timezone=Europe%2FBerlin&forecast_days=1";
String trafficURL = "https://www.skanetrafiken.se/gw-tps/api/v2/Journey?fromPointId=9021012093070000&fromPointType=STOP_AREA&toPointId=9021012080040000&toPointType=STOP_AREA&arrival=false&priority=SHORTEST_TIME&journeysAfter=4&walkSpeed=NORMAL&maxWalkDistance=2000&allowWalkToOtherStop=true";

double precipitation;

double parseDouble(String jsonString) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, jsonString);
  if (err) {
    Serial.print("Deserialization failed!");
    Serial.println(err.f_str());
    return -1;
  }
  return doc["daily"]["precipitation_sum"][0];
}

String httpGETRequest(String endpoint) {
  WiFiClient client;
  HTTPClient http;

  http.begin(client, endpoint.c_str());
  int httpResponseCode = http.GET();

  String payload = "{}";
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
  Serial.println(payload);
  return payload;
}

double getPrecipitation() {
  String wttr = httpGETRequest(weatherURL);
  return parseDouble(wttr);
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
  delay(5000);
  Serial.println('\n');

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

  String payload = "{}";
  WiFiClientSecure *client = new WiFiClientSecure;
  if(client) {
    // set secure client without certificate
    client->setInsecure();
    //create an HTTPClient instance
    HTTPClient https;

    //Initializing an HTTPS communication using the secure client
    Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, weatherURL.c_str())) {  // HTTPS
      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      int httpCode = https.GET();
      // httpCode will be negative on error
      if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
       Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
      // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          // print server response payload
          payload = https.getString();
        }
      }
      else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
    }
  }
  else {
    Serial.printf("[HTTPS] Unable to connect\n");
  }


  precipitation = parseDouble(payload);

  digitalWrite(GREEN_PIN,HIGH);
  if (precipitation > 10) {
    digitalWrite(YELLOW_PIN,HIGH);
  }
}

void loop() {
  Serial.println(precipitation);
  delay(5000);
}
