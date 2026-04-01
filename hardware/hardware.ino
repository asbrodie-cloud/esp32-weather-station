#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <SPI.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>

// =========================
// Wi-Fi Credentials
// =========================
const char* ssid = "MSI_25H";
const char* password = "50A5DC8209FF";

// =========================
// MQTT Settings
// =========================
const char* mqtt_server = "www.yanacreations.com";   // replace with your PC IP or broker IP
const int mqtt_port = 1883;
const char* mqtt_pub_topic = "weatherstation/data";
const char* mqtt_sub_topic = "weatherstation/test";
const char* device_id = "ESP32_01";
const char* mqtt_cmd_topic = "weatherstation/control";


// Manual control states
bool acManual = false;
bool acState = false;

bool sprinklerManual = false;
bool sprinklerState = false;

bool dehumidifierManual = false;
bool dehumidifierState = false;

// =========================
// Pin Definitions
// =========================
#define DHTPIN   4
#define DHTTYPE  DHT22
#define SOIL_PIN 34

#define TFT_CS   5
#define TFT_DC   27
#define TFT_RST  33
#define TFT_MOSI 23
#define TFT_CLK  18
#define TFT_MISO 19
#define TFT_LED  32

#define I2C_SDA  21
#define I2C_SCL  22

// =========================
// Objects
// =========================
WiFiClient espClient;
PubSubClient mqttClient(espClient);

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
WebServer server(80);

// =========================
// Globals
// =========================
float temperature = 0.0;
float humidity = 0.0;
float pressure_hPa = 0.0;
float soilPercent = 0.0;
float heatIndexC = 0.0;
float altitudeM = 0.0;
int soilRaw = 0;

float lastValidTemp = 0.0;
float lastValidHum = 0.0;
float lastValidPressure = 0.0;
float lastValidSoilPercent = 0.0;
float lastValidHeatIndexC = 0.0;
float lastValidAltitudeM = 0.0;

bool dhtOk = false;
bool bmpOk = false;

unsigned long lastSensorRead = 0;
unsigned long lastMqttPublish = 0;

const unsigned long readInterval = 3000;
const unsigned long publishInterval = 5000;

const int SOIL_DRY = 3200;
const int SOIL_WET = 1100;
const float SEA_LEVEL_HPA = 1013.25;

bool mqttAvailable = false;
unsigned long lastMqttReconnectAttempt = 0;
const unsigned long mqttReconnectInterval = 5000; // try every 5 seconds

// =========================
// Function Prototypes
// =========================
void connectWiFi();
bool reconnectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void setupWebServer();
void readSensors();
void publishSensorData();
void drawDashboard();
String getHTMLPage();

void drawHeader();
void drawStatusBar();
void drawBox(int x, int y, int w, int h, uint16_t color, const char* label, const String& value, const char* unit);
void drawThermometerIcon(int x, int y, uint16_t color);
void drawDropIcon(int x, int y, uint16_t color);
void drawPressureIcon(int x, int y, uint16_t color);
void drawLeafIcon(int x, int y, uint16_t color);
void drawSoilBar(int x, int y, int w, int h, float percent);
void showStartupScreen(const char* line1, const char* line2, uint16_t color);

// =========================
// Setup
// =========================
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);

  Wire.begin(I2C_SDA, I2C_SCL);
  dht.begin();

  SPI.begin(TFT_CLK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.begin();
  tft.setRotation(1);

  showStartupScreen("Weather Station", "Initializing...", ILI9341_CYAN);

  if (bmp.begin(0x76)) {
    bmpOk = true;
    Serial.println("BMP280 found at 0x76");
  } else if (bmp.begin(0x77)) {
    bmpOk = true;
    Serial.println("BMP280 found at 0x77");
  } else {
    bmpOk = false;
    Serial.println("BMP280 not found!");
  }

  if (bmpOk) {
    bmp.setSampling(
      Adafruit_BMP280::MODE_NORMAL,
      Adafruit_BMP280::SAMPLING_X2,
      Adafruit_BMP280::SAMPLING_X16,
      Adafruit_BMP280::FILTER_X16,
      Adafruit_BMP280::STANDBY_MS_500
    );
  }

  connectWiFi();

  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);

  setupWebServer();

  readSensors();
  drawDashboard();
}

// =========================
// Main Loop
// =========================
void loop() {
  // Keep web server running
  server.handleClient();

  // Keep Wi-Fi alive
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // MQTT should NOT block the rest of the system
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      unsigned long now = millis();
      if (now - lastMqttReconnectAttempt >= mqttReconnectInterval) {
        lastMqttReconnectAttempt = now;
        reconnectMQTT();
      }
    } else {
      mqttClient.loop();
      mqttAvailable = true;
    }
  } else {
    mqttAvailable = false;
  }

  // Sensor reading and TFT update must continue no matter what
  if (millis() - lastSensorRead >= readInterval) {
    lastSensorRead = millis();
    readSensors();
    drawDashboard();
  }

  // Publish only if MQTT is available
  if (mqttAvailable && millis() - lastMqttPublish >= publishInterval) {
    lastMqttPublish = millis();
    publishSensorData();
  }
}

// =========================
// Wi-Fi
// =========================
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  showStartupScreen("Connecting Wi-Fi", "Please wait...", ILI9341_YELLOW);

  Serial.print("Connecting to Wi-Fi");
  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi connection failed");
  }
}

// =========================
// MQTT
// =========================
bool reconnectMQTT() {
  Serial.print("Attempting MQTT connection...");

  String clientId = "ESP32Client-";
  clientId += String(random(0xffff), HEX);

  if (mqttClient.connect(clientId.c_str())) {
    Serial.println("connected");
    mqttClient.subscribe(mqtt_sub_topic);
    mqttClient.subscribe(mqtt_cmd_topic);
    Serial.print("Subscribed to: ");
    Serial.println(mqtt_sub_topic);
    mqttAvailable = true;
    return true;
  } else {
    Serial.print("failed, rc=");
    Serial.println(mqttClient.state());
    mqttAvailable = false;
    return false;
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;

  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Message received on topic ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(message);

  if (String(topic) == mqtt_cmd_topic) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
      Serial.println("Failed to parse control JSON");
      return;
    }

    String device = doc["device"] | "";
    String mode = doc["mode"] | "";
    bool state = doc["state"] | false;

    if (device == "ac") {
      if (mode == "auto") {
        acManual = false;
      } else if (mode == "manual") {
        acManual = true;
        acState = state;
      }
    }

    if (device == "sprinkler") {
      if (mode == "auto") {
        sprinklerManual = false;
      } else if (mode == "manual") {
        sprinklerManual = true;
        sprinklerState = state;
      }
    }

    if (device == "dehumidifier") {
      if (mode == "auto") {
        dehumidifierManual = false;
      } else if (mode == "manual") {
        dehumidifierManual = true;
        dehumidifierState = state;
      }
    }

    Serial.println("Control state updated");
  }
}

void publishSensorData() {
  if (!mqttClient.connected()) {
    mqttAvailable = false;
    return;
  }

  StaticJsonDocument<256> doc;

  doc["device_id"] = device_id;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["pressure"] = pressure_hPa;
  doc["soil_moisture"] = soilPercent;
  doc["soil_raw"] = soilRaw;
  doc["heat_index"] = heatIndexC;
  doc["altitude"] = altitudeM;
  doc["timestamp"] = millis();
  doc["ac_state"] = acState;
  doc["ac_manual"] = acManual;
  doc["sprinkler_state"] = sprinklerState;
  doc["sprinkler_manual"] = sprinklerManual;
  doc["dehumidifier_state"] = dehumidifierState;
  doc["dehumidifier_manual"] = dehumidifierManual;

  char buffer[256];
  serializeJson(doc, buffer);

  if (mqttClient.publish(mqtt_pub_topic, buffer)) {
    Serial.println("MQTT publish successful");
    Serial.println(buffer);
    mqttAvailable = true;
  } else {
    Serial.println("MQTT publish failed");
    mqttAvailable = false;
  }
}

// =========================
// Web Server
// =========================
void setupWebServer() {
  server.on("/", []() {
    server.send(200, "text/html", getHTMLPage());
  });

  server.begin();
}

// =========================
// Sensor Reading
// =========================
void readSensors() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (!isnan(t) && !isnan(h)) {
    temperature = t;
    humidity = h;
    heatIndexC = dht.computeHeatIndex(t, h, false);
    lastValidTemp = temperature;
    lastValidHum = humidity;
    lastValidHeatIndexC = heatIndexC;
    dhtOk = true;
  } else {
    temperature = lastValidTemp;
    humidity = lastValidHum;
    heatIndexC = lastValidHeatIndexC;
    dhtOk = false;
  }

  if (bmpOk) {
    float p = bmp.readPressure() / 100.0F;
    float alt = bmp.readAltitude(SEA_LEVEL_HPA);

    if (!isnan(p) && p > 300.0 && p < 1200.0) {
      pressure_hPa = p;
      altitudeM = alt;
      lastValidPressure = pressure_hPa;
      lastValidAltitudeM = altitudeM;
    } else {
      pressure_hPa = lastValidPressure;
      altitudeM = lastValidAltitudeM;
    }
  } else {
    pressure_hPa = lastValidPressure;
    altitudeM = lastValidAltitudeM;
  }

  soilRaw = analogRead(SOIL_PIN);

  float mapped = map(soilRaw, SOIL_DRY, SOIL_WET, 0, 100);
  if (mapped < 0) mapped = 0;
  if (mapped > 100) mapped = 100;

  soilPercent = mapped;
  lastValidSoilPercent = soilPercent;

    // Automatic logic if not in manual mode
  if (!acManual) {
    acState = (temperature >= 28.0);
  }

  if (!sprinklerManual) {
    sprinklerState = (soilPercent <= 30.0);
  }

  if (!dehumidifierManual) {
    dehumidifierState = (humidity >= 70.0);
  }
}

// =========================
// TFT Dashboard
// =========================
void drawDashboard() {
  tft.fillScreen(ILI9341_BLACK);

  drawHeader();

  String tempText = dhtOk ? String(temperature, 1) : "ERR";
  String humText  = dhtOk ? String(humidity, 1) : "ERR";
  String presText = bmpOk ? String(pressure_hPa, 1) : "ERR";
  String soilText = String((int)soilPercent);

  drawBox(10, 40, 145, 72, ILI9341_RED, "Temp", tempText, "C");
  drawThermometerIcon(130, 54, ILI9341_RED);

  drawBox(165, 40, 145, 72, ILI9341_CYAN, "Hum", humText, "%");
  drawDropIcon(285, 60, ILI9341_CYAN);

  drawBox(10, 118, 145, 72, ILI9341_YELLOW, "Press", presText, "hPa");
  drawPressureIcon(130, 140, ILI9341_YELLOW);

  drawBox(165, 118, 145, 72, ILI9341_GREEN, "Soil", soilText, "%");
  drawLeafIcon(285, 140, ILI9341_GREEN);
  drawSoilBar(178, 172, 118, 10, soilPercent);

  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(1);

  tft.setCursor(10, 198);
  tft.print("HI:");
  tft.print(heatIndexC, 1);
  tft.print("C");

  tft.setCursor(95, 198);
  tft.print("ALT:");
  tft.print(altitudeM, 1);
  tft.print("m");

  tft.setCursor(205, 198);
  tft.print("Raw:");
  tft.print(soilRaw);

  drawStatusBar();
}

void drawHeader() {
  tft.fillRect(0, 0, 320, 30, ILI9341_BLUE);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLUE);
  tft.setTextSize(2);
  tft.setCursor(65, 8);
  tft.print("Weather Station");
}

void drawStatusBar() {
  tft.fillRect(0, 220, 320, 20, ILI9341_DARKCYAN);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE, ILI9341_DARKCYAN);
  tft.setCursor(8, 226);

  if (WiFi.status() == WL_CONNECTED && mqttAvailable) {
    tft.print("WiFi OK | MQTT OK");
  } else if (WiFi.status() == WL_CONNECTED && !mqttAvailable) {
    tft.print("WiFi OK | MQTT OFF");
  } else {
    tft.print("Offline");
  }
}

void drawBox(int x, int y, int w, int h, uint16_t color, const char* label, const String& value, const char* unit) {
  tft.drawRoundRect(x, y, w, h, 8, color);
  tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 8, color);

  tft.setTextColor(color, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(x + 10, y + 8);
  tft.print(label);

  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(3);
  tft.setCursor(x + 10, y + 32);
  tft.print(value);

  if (String(unit) == "hPa") {
    tft.setTextSize(1);
    tft.setCursor(x + 10, y + 58);
    tft.print(unit);
  } else {
    tft.setTextSize(2);
    tft.setCursor(x + w - 25, y + 45);
    tft.print(unit);
  }
}

void drawThermometerIcon(int x, int y, uint16_t color) {
  tft.drawCircle(x, y + 18, 8, color);
  tft.fillCircle(x, y + 18, 6, color);
  tft.fillRect(x - 3, y, 6, 18, color);
  tft.drawRect(x - 5, y - 2, 10, 24, color);
}

void drawDropIcon(int x, int y, uint16_t color) {
  tft.fillTriangle(x, y - 10, x - 8, y + 5, x + 8, y + 5, color);
  tft.fillCircle(x, y + 8, 8, color);
}

void drawPressureIcon(int x, int y, uint16_t color) {
  tft.drawCircle(x, y, 12, color);
  tft.drawLine(x, y, x + 8, y - 5, color);
  tft.fillCircle(x, y, 2, color);
}

void drawLeafIcon(int x, int y, uint16_t color) {
  tft.drawCircle(x - 4, y, 6, color);
  tft.drawCircle(x + 4, y, 6, color);
  tft.drawLine(x - 8, y + 5, x + 8, y - 5, color);
}

void drawSoilBar(int x, int y, int w, int h, float percent) {
  tft.drawRect(x, y, w, h, ILI9341_WHITE);
  int fillW = (int)((percent / 100.0) * (w - 2));
  uint16_t fillColor = ILI9341_GREEN;

  if (percent < 30) fillColor = ILI9341_RED;
  else if (percent < 60) fillColor = ILI9341_YELLOW;

  tft.fillRect(x + 1, y + 1, fillW, h - 2, fillColor);
}

void showStartupScreen(const char* line1, const char* line2, uint16_t color) {
  tft.fillScreen(ILI9341_BLACK);
  tft.fillRect(0, 0, 320, 40, color);

  tft.setTextColor(ILI9341_WHITE, color);
  tft.setTextSize(2);
  tft.setCursor(45, 12);
  tft.print(line1);

  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(40, 100);
  tft.print(line2);
}

String getHTMLPage() {
  String html = "<html><body><h1>ESP32 Weather Station</h1>";
  html += "<p>Temperature: " + String(temperature, 1) + " C</p>";
  html += "<p>Humidity: " + String(humidity, 1) + " %</p>";
  html += "<p>Pressure: " + String(pressure_hPa, 1) + " hPa</p>";
  html += "<p>Soil Moisture: " + String(soilPercent, 1) + " %</p>";
  html += "<p>Heat Index: " + String(heatIndexC, 1) + " C</p>";
  html += "<p>Altitude: " + String(altitudeM, 1) + " m</p>";
  html += "</body></html>";
  return html;
}