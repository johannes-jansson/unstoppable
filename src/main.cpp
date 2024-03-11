#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

#define GREEN_PIN 13
#define YELLOW_PIN 12
#define RED_PIN 14

const char* ssid     = "The Promised LAN";       
const char* password = ""; 

String weatherURL = "https://wttr.in/H%C3%A4ssleholm?format=j1";
String trafficURL = "https://www.skanetrafiken.se/gw-tps/api/v2/Journey?fromPointId=9021012093070000&fromPointType=STOP_AREA&toPointId=9021012080040000&toPointType=STOP_AREA&arrival=false&priority=SHORTEST_TIME&journeysAfter=1&walkSpeed=NORMAL&maxWalkDistance=2000&allowWalkToOtherStop=true";

int precipitation = -2;
int deviation;
int precipitation_threshold = 1;
unsigned long timer;

// Go through every hour of current day, and if precipitation is ever greater
// than threshold, return 1. If not, return 0.
int parsePrecipitation(String jsonString, int threshold) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, jsonString);
  if (err) {
    Serial.print("Deserialization failed!");
    Serial.println(err.f_str());
    return -1;
  }
  JsonArray hourly = doc["weather"][0]["hourly"];
  for (int i = 0; i < hourly.size(); i++) {
    if (hourly[i]["precipMM"] > threshold) {
      return 1;
    }
  }
  return 0;
}

// Go through every journey in the result, and if there are any deviations,
// return 1. If not, return 0. If no journeys are found, return -1.
int parseTraffic(String jsonString) {
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

// Make a https-request and return the payload as a string
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
  digitalWrite(GREEN_PIN, LOW);
  // Fetch and parse weather only once, since it doesn't change that rapidly
  if (precipitation == -2) {
    String weatherPayload = httpsGETRequest(weatherURL);
    precipitation = parsePrecipitation(weatherPayload, precipitation_threshold);
    if (precipitation > 0) {
      digitalWrite(YELLOW_PIN,HIGH);
    }
  }

  // Fetch and parse traffic on each update
  String trafficPayload = httpsGETRequest(trafficURL);
  deviation = parseTraffic(trafficPayload);

  digitalWrite(GREEN_PIN, HIGH);
  if (deviation > 0) {
    digitalWrite(RED_PIN, HIGH);
  }
  if (precipitation < 0 || deviation < 0) {
    digitalWrite(GREEN_PIN,LOW);
  }
}


void setup() {
  // Prepare the timer
  timer = millis();

  // Prepare hardware pins
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
  digitalWrite(GREEN_PIN,LOW);
  digitalWrite(YELLOW_PIN,LOW);
  digitalWrite(RED_PIN,LOW);
  
  // Prepare serial comms
  Serial.begin(115200);         
  delay(10);
  delay(5000);
  Serial.println('\n');

  // Prepare wifi
  setupWiFi();
}

void loop() {
  // After an hour in the loop, sleep forever (= wake up only on reset button)
  if (millis() - timer > 60 * 60 * 1000) {
    Serial.flush();
    esp_deep_sleep_start();
  }
  update();
  // Wait three minutes
  delay(3 * 60 * 1000);
}
