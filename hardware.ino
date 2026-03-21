#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>

// =========================
// Wi-Fi Credentials
// =========================
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// =========================
// Pin Definitions
// =========================
#define DHTPIN   4
#define DHTTYPE  DHT22

#define SOIL_PIN 34   // ADC1 pin, safe with Wi-Fi

// TFT Display (ILI9341 SPI)
#define TFT_CS   5
#define TFT_DC   27
#define TFT_RST  33
#define TFT_MOSI 23
#define TFT_CLK  18
#define TFT_MISO 19
#define TFT_LED  32

// I2C for BMP280
#define I2C_SDA  21
#define I2C_SCL  22

// =========================
// Objects
// =========================
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp; // I2C
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
const unsigned long readInterval = 3000; // 3 seconds

// Soil calibration values
// Adjust after testing your sensor
const int SOIL_DRY = 3200;   // dry soil raw value
const int SOIL_WET = 1400;   // wet soil raw value

// Sea-level pressure for altitude estimate
const float SEA_LEVEL_HPA = 1013.25;

// =========================
// Function Prototypes
// =========================
void connectWiFi();
void setupWebServer();
void readSensors();
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
  tft.setRotation(1); // landscape

  showStartupScreen("Weather Station", "Initializing...", ILI9341_CYAN);

  // BMP280 init
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
  setupWebServer();

  readSensors();
  drawDashboard();
}

// =========================
// Main Loop
// =========================
void loop() {
  server.handleClient();

  if (millis() - lastSensorRead >= readInterval) {
    lastSensorRead = millis();
    readSensors();
    drawDashboard();
  }
}

// =========================
// Wi-Fi Connection
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

    tft.fillScreen(ILI9341_BLACK);
    tft.fillRect(0, 0, 320, 40, ILI9341_GREEN);
    tft.setTextColor(ILI9341_WHITE, ILI9341_GREEN);
    tft.setTextSize(2);
    tft.setCursor(50, 12);
    tft.print("Wi-Fi Connected");

    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setTextSize(2);
    tft.setCursor(20, 80);
    tft.print("IP Address:");
    tft.setCursor(20, 110);
    tft.print(WiFi.localIP());

    delay(1800);
  } else {
    Serial.println("Wi-Fi connection failed");

    tft.fillScreen(ILI9341_BLACK);
    tft.fillRect(0, 0, 320, 40, ILI9341_RED);
    tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
    tft.setTextSize(2);
    tft.setCursor(70, 12);
    tft.print("Wi-Fi Failed");

    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setCursor(20, 80);
    tft.print("Running offline mode");
    delay(1800);
  }
}

// =========================
// Web Server
// =========================
void setupWebServer() {
  server.on("/", []() {
    server.send(200, "text/html", getHTMLPage());
  });

  server.on("/data", []() {
    String json = "{";
    json += "\"temperature\":" + String(temperature, 1) + ",";
    json += "\"humidity\":" + String(humidity, 1) + ",";
    json += "\"heatIndex\":" + String(heatIndexC, 1) + ",";
    json += "\"pressure\":" + String(pressure_hPa, 1) + ",";
    json += "\"altitude\":" + String(altitudeM, 1) + ",";
    json += "\"soilRaw\":" + String(soilRaw) + ",";
    json += "\"soilPercent\":" + String(soilPercent, 1) + ",";
    json += "\"dhtOk\":" + String(dhtOk ? "true" : "false") + ",";
    json += "\"bmpOk\":" + String(bmpOk ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
  });

  server.begin();
  Serial.println("Web server started");
}

// =========================
// Read Sensors
// =========================
void readSensors() {
  // DHT22
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (!isnan(t) && !isnan(h)) {
    temperature = t;
    humidity = h;
    heatIndexC = dht.computeHeatIndex(t, h, false); // Celsius
    lastValidTemp = temperature;
    lastValidHum = humidity;
    lastValidHeatIndexC = heatIndexC;
    dhtOk = true;
  } else {
    temperature = lastValidTemp;
    humidity = lastValidHum;
    heatIndexC = lastValidHeatIndexC;
    dhtOk = false;
    Serial.println("DHT22 read failed");
  }

  // BMP280
  if (bmpOk) {
    float p = bmp.readPressure() / 100.0F; // Pa to hPa
    float alt = bmp.readAltitude(SEA_LEVEL_HPA);

    if (!isnan(p) && p > 300.0 && p < 1200.0) {
      pressure_hPa = p;
      altitudeM = alt;
      lastValidPressure = pressure_hPa;
      lastValidAltitudeM = altitudeM;
    } else {
      pressure_hPa = lastValidPressure;
      altitudeM = lastValidAltitudeM;
      Serial.println("BMP280 reading invalid");
    }
  } else {
    pressure_hPa = lastValidPressure;
    altitudeM = lastValidAltitudeM;
  }

  // Soil sensor
  soilRaw = analogRead(SOIL_PIN);

  float mapped = map(soilRaw, SOIL_DRY, SOIL_WET, 0, 100);
  if (mapped < 0) mapped = 0;
  if (mapped > 100) mapped = 100;

  soilPercent = mapped;
  lastValidSoilPercent = soilPercent;

  // Serial output
  Serial.println("----------- Sensor Data -----------");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.print("Heat Index: ");
  Serial.print(heatIndexC);
  Serial.println(" C");

  Serial.print("Pressure: ");
  Serial.print(pressure_hPa);
  Serial.println(" hPa");

  Serial.print("Altitude: ");
  Serial.print(altitudeM);
  Serial.println(" m");

  Serial.print("Soil Raw: ");
  Serial.println(soilRaw);

  Serial.print("Soil Moisture: ");
  Serial.print(soilPercent);
  Serial.println(" %");

  Serial.println("-----------------------------------");
}

// =========================
// Dashboard Drawing
// =========================
void drawDashboard() {
  tft.fillScreen(ILI9341_BLACK);

  drawHeader();

  String tempText = dhtOk ? String(temperature, 1) : "ERR";
  String humText  = dhtOk ? String(humidity, 1) : "ERR";
  String presText = bmpOk ? String(pressure_hPa, 1) : "ERR";
  String soilText = String((int)soilPercent);

  // Top row
  drawBox(10, 40, 145, 72, ILI9341_RED,   "Temp",  tempText, "C");
  drawThermometerIcon(130, 54, ILI9341_RED);

  drawBox(165, 40, 145, 72, ILI9341_CYAN, "Hum",   humText,  "%");
  drawDropIcon(285, 60, ILI9341_CYAN);

  // Bottom row
  drawBox(10, 118, 145, 72, ILI9341_YELLOW, "Press", presText, "hPa");
  drawPressureIcon(130, 140, ILI9341_YELLOW);

  drawBox(165, 118, 145, 72, ILI9341_GREEN, "Soil", soilText, "%");
  drawLeafIcon(285, 140, ILI9341_GREEN);
  drawSoilBar(178, 172, 118, 10, soilPercent);

  // Extra info line
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

  if (WiFi.status() == WL_CONNECTED) {
    tft.fillCircle(8, 230, 4, ILI9341_GREEN);
    tft.setTextColor(ILI9341_WHITE, ILI9341_DARKCYAN);
    tft.setCursor(18, 226);
    tft.print("WiFi Connected ");
    tft.print(WiFi.localIP());
  } else {
    tft.fillCircle(8, 230, 4, ILI9341_RED);
    tft.setTextColor(ILI9341_WHITE, ILI9341_DARKCYAN);
    tft.setCursor(18, 226);
    tft.print("WiFi Not Connected");
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

  if (percent < 30) {
    fillColor = ILI9341_RED;
  } else if (percent < 60) {
    fillColor = ILI9341_YELLOW;
  }

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

// =========================
// Web Page HTML
// =========================
String getHTMLPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta http-equiv="refresh" content="5">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Weather Station</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      background: #f4f6f8;
      margin: 0;
      padding: 20px;
    }
    .container {
      max-width: 760px;
      margin: auto;
    }
    .card {
      background: white;
      padding: 20px;
      margin: 15px 0;
      border-radius: 12px;
      box-shadow: 0 2px 8px rgba(0,0,0,0.1);
    }
    h1 {
      color: #222;
    }
    .value {
      font-size: 2rem;
      font-weight: bold;
      margin-top: 10px;
    }
    .label {
      font-size: 1rem;
      color: #666;
    }
    .footer {
      margin-top: 20px;
      color: #777;
      font-size: 0.9rem;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP32 Weather Station</h1>

    <div class="card">
      <div class="label">Temperature</div>
      <div class="value">)rawliteral";

  html += String(temperature, 1) + " &deg;C";

  html += R"rawliteral(</div>
    </div>

    <div class="card">
      <div class="label">Humidity</div>
      <div class="value">)rawliteral";

  html += String(humidity, 1) + " %";

  html += R"rawliteral(</div>
    </div>

    <div class="card">
      <div class="label">Heat Index</div>
      <div class="value">)rawliteral";

  html += String(heatIndexC, 1) + " &deg;C";

  html += R"rawliteral(</div>
    </div>

    <div class="card">
      <div class="label">Pressure</div>
      <div class="value">)rawliteral";

  html += String(pressure_hPa, 1) + " hPa";

  html += R"rawliteral(</div>
    </div>

    <div class="card">
      <div class="label">Altitude</div>
      <div class="value">)rawliteral";

  html += String(altitudeM, 1) + " m";

  html += R"rawliteral(</div>
    </div>

    <div class="card">
      <div class="label">Soil Moisture</div>
      <div class="value">)rawliteral";

  html += String(soilPercent, 1) + " %";

  html += R"rawliteral(</div>
    </div>

    <div class="footer">
      Refreshes every 5 seconds
    </div>
  </div>
</body>
</html>
)rawliteral";

  return html;
}