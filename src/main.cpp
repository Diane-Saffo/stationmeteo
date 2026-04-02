#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
const char* ssid       = "UNIFI_IDO1";
const char* password   = "42Bidules!";

const char* apiUrl     = "https://api.openweathermap.org/data/2.5/weather?q=Bathurst,CA&appid=fa31e5defae6296fbac88c429099a1fc&units=metric&lang=fr";


WebServer server(80);

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void afficherOLED(String l1, String l2 = "", String l3 = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);  display.println(l1);
  display.setCursor(0, 16); display.println(l2);
  display.setCursor(0, 32); display.println(l3);
  display.display();
}


void handleRoot() {
  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    server.send(500, "text/plain", "index.html manquant");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
}

void handleScript() {
  File file = SPIFFS.open("/script.js", "r");
  if (!file) {
    server.send(500, "text/plain", "script.js manquant");
    return;
  }
  server.streamFile(file, "application/javascript");
  file.close();
}

void handleMeteoAPI() {
  HTTPClient http;
  http.begin(apiUrl);
  int code = http.GET();

  if (code <= 0) {
    server.send(500, "application/json", "{\"error\":\"HTTP error\"}");
    return;
  }

  String payload = http.getString();
  http.end();

  server.send(200, "application/json", payload);

  StaticJsonDocument<1024> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) return;

  float temperature = doc["main"]["temp"] | 0.0;
  float humidite    = doc["main"]["humidity"] | 0.0;
  float pression    = doc["main"]["pressure"] | 0.0;
  float vent        = doc["wind"]["speed"] | 0.0;
  const char* ville = doc["name"] | "Ville";

  String l1 = String(ville);
  String l2 = "T:" + String(temperature, 1) + "C H:" + String(humidite, 0) + "%";
  String l3 = "P:" + String(pression, 0) + "hPa V:" + String(vent, 1) + "km/h";
  afficherOLED(l1, l2, l3);
}


void connecterWiFi() {
  WiFi.begin(ssid, password);
  afficherOLED("Connexion WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  afficherOLED("WiFi OK", WiFi.localIP().toString());

}
void setup() {
  Serial.begin(115200);

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true) delay(1000);
  }
  afficherOLED("Boot ESP32...");

  
  if (!SPIFFS.begin(true)) {
    afficherOLED("SPIFFS ERROR");
    return;
  }

  connecterWiFi();

  server.on("/", handleRoot);
  server.on("/script.js", handleScript);
  server.on("/api/meteo", handleMeteoAPI);

  server.begin();
  afficherOLED("Serveur OK", WiFi.localIP().toString());
}

void loop() {
  server.handleClient();
}