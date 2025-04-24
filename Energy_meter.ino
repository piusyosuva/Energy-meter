// =======================================================
// ESP32 Power Monitor (SPI OLED + Google Sheets)
// Domestic LT-1 Slab (Tamil Nadu) | Full Feature Set
// =======================================================

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_INA219.h>
#include <WiFi.h>
#include <HTTPClient.h>

// --------- Configuration ---------
const char* WIFI_SSID     = "Yod";
const char* WIFI_PASSWORD = "Bax@2023";
const char* SCRIPT_URL    = "https://script.google.com/macros/s/AKfycbyo5nianVYv295WAroDOsFW-Mc32yCfhD6oyvpleZCGb-vsbJFBkAKkWKgWbpKbbce-/exec";

// --------- OLED (SPI) Pins ---------
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_DC       17
#define OLED_RESET    16
#define OLED_CS       5
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// --------- INA219 Sensor ---------
Adafruit_INA219 ina219;

// --------- Timing Constants ---------
const unsigned long MS_PER_SEC = 1000UL;
const unsigned long SEC_PER_HR = 3600UL;

// --------- Energy Tracking ---------
float hourlyWh        = 0.0f;            // accumulated Wh in current hour
float dailyWhArray[24]   = {0};           // total Wh each hour
float hourlyMeanArray[24] = {0};          // mean Wh per hour
int   hourIndex       = 0;               // 0–23
unsigned long secCount = 0;              // seconds into current hour
unsigned long lastMillis;

// --------- Function Prototypes ---------
void connectToWiFi();
void testHTTP();
void readAndAccumulate();
void sendSecondToSheet(int hour, unsigned long duration, float voltage, float current_mA, float power_W, float whr);
void handleHourRollover();
void handleDayRollover();
void sendHourlyMeanToSheet(int hour, float meanWh);
void sendDailySummary(float unitsDay, float costDay, float slab);
float calculateDomesticBill(float units);

// =======================================================
// setup()
// =======================================================
void setup() {
  Serial.begin(115200);

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println("ERROR: OLED init failed");
    while (true);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // INA219 init
  ina219.begin();

  // WiFi + HTTP test banner
  connectToWiFi();
  testHTTP();

  // start timer
  lastMillis = millis();
}

// =======================================================
// loop()
// =======================================================
void loop() {
  unsigned long now = millis();

  // 1) sample every second
  if (now - lastMillis >= MS_PER_SEC) {
    lastMillis += MS_PER_SEC;
    readAndAccumulate();
  }

  // 2) hour rollover
  if (secCount >= SEC_PER_HR) handleHourRollover();

  // 3) day rollover
  if (hourIndex >= 24) handleDayRollover();
}

// =======================================================
// connectToWiFi(): join network & show on OLED
// =======================================================
void connectToWiFi() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Connecting to WiFi...");
  display.display();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("WiFi Connected ✔");
  display.display();
  delay(500);
}

// =======================================================
// testHTTP(): simple GET to confirm SCRIPT_URL works
// =======================================================
void testHTTP() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Testing HTTP...");
  display.display();

  HTTPClient http;
  String url = String(SCRIPT_URL) + "?test=ping";
  http.begin(url);

  int code = http.GET();
  http.end();

  display.clearDisplay();
  display.setCursor(0,0);
  display.printf("HTTP test code:\n%d", code);
  display.display();
  delay(3000);
}

// =======================================================
// readAndAccumulate(): called every second
//   - measures V, I, P, Whr, duration
//   - logs it to sheet
//   - updates OLED display
// =======================================================
void readAndAccumulate() {
  float V   = ina219.getBusVoltage_V();
  float mA  = ina219.getCurrent_mA();
  float P   = V * (mA / 1000.0f);        // W
  float whr = P / SEC_PER_HR;            // Wh consumed in this 1s

  secCount++;
  hourlyWh += whr;

  // Display all five metrics on OLED
  display.clearDisplay();
  display.setCursor(0,0);
  display.printf("V:%.2f V\nI:%.1f mA\nP:%.2f W\nWhr:%.4f\nT: %lus\n", 
                 V, mA, P, whr, secCount);
  display.display();

  // Send to Google Sheet
  sendSecondToSheet(hourIndex+1, secCount, V, mA, P, whr);
}

// =======================================================
// sendSecondToSheet(): log per-second data
// =======================================================
void sendSecondToSheet(int hour, unsigned long duration,
                       float voltage, float current_mA,
                       float power_W, float whr) {
  if (WiFi.status() != WL_CONNECTED) return;
  String url = String(SCRIPT_URL)
    + "?type=second"
    + "&hour="     + String(hour)
    + "&duration=" + String(duration)
    + "&voltage="  + String(voltage,3)
    + "&current="  + String(current_mA,3)
    + "&power="    + String(power_W,4)
    + "&whr="      + String(whr,6);
  Serial.println(url);
  HTTPClient http; 
  http.begin(url);
  int code = http.GET();
  Serial.println(" code:" + String(code));
  http.end();
}

// =======================================================
// handleHourRollover(): compute mean, log, reset
// =======================================================
void handleHourRollover() {
  dailyWhArray[hourIndex]    = hourlyWh;
  float meanWh               = hourlyWh / (float)SEC_PER_HR;
  hourlyMeanArray[hourIndex] = meanWh;

  sendHourlyMeanToSheet(hourIndex+1, meanWh);

  hourIndex++;
  secCount   = 0;
  hourlyWh   = 0.0f;
}

// =======================================================
// sendHourlyMeanToSheet(): log Whr-per-hour
// =======================================================
void sendHourlyMeanToSheet(int hour, float meanWh) {
  if (WiFi.status() != WL_CONNECTED) return;
  String url = String(SCRIPT_URL)
    + "?type=hourly"
    + "&hourMean=" + String(hour)
    + "&meanWh="   + String(meanWh,4);
  Serial.println(url);
  HTTPClient http; http.begin(url);
  int code = http.GET(); Serial.println(" code:"+String(code));
  http.end();
}

// =======================================================
// handleDayRollover(): compute daily summary, log, display
// =======================================================
void handleDayRollover() {
  float totalWh = 0;
  for (int i=0;i<24;i++) totalWh += dailyWhArray[i];
  float unitsDay = totalWh/1000.0f;
  float avgMean  = 0;
  for (int i=0;i<24;i++) avgMean += hourlyMeanArray[i];
  avgMean /= 24.0f;
  float cost = calculateDomesticBill(unitsDay);

  sendDailySummary(unitsDay, cost, avgMean);

  display.clearDisplay();
  display.setCursor(0,0);
  display.printf("24h Total: %.2f Wh\nUnits: %.3f kWh\nCost: Rs₹%.2f\nAvgHr: %.4f Wh", 
                 totalWh, unitsDay, cost, avgMean);
  display.display();
  delay(10000);

  memset(dailyWhArray,0,sizeof(dailyWhArray));
  memset(hourlyMeanArray,0,sizeof(hourlyMeanArray));
  hourIndex=0; secCount=0; hourlyWh=0;
}

// =======================================================
// sendDailySummary(): log daily & slab
// =======================================================
void sendDailySummary(float uDay, float cost, float avgHr) {
  if (WiFi.status() != WL_CONNECTED) return;
  String url = String(SCRIPT_URL)
    + "?type=daily"
    + "&unitsDay=" + String(uDay,3)
    + "&cost="     + String(cost,2)
    + "&avgHr="    + String(avgHr,4);
  Serial.println(url);
  HTTPClient http; http.begin(url);
  int code = http.GET(); Serial.println(" code:"+String(code));
  http.end();
}

// =======================================================
// calculateDomesticBill(): Tamil Nadu LT-1
// =======================================================
float calculateDomesticBill(float units) {
  if      (units<=100) return units*2.5f;
  else if (units<=200) return 100*2.5f + (units-100)*4.5f;
  else if (units<=400) return 100*2.5f+100*4.5f+(units-200)*5.5f;
  else if (units<=500) return 100*2.5f+100*4.5f+200*5.5f+(units-400)*6.0f;
  else                 return 100*2.5f+100*4.5f+200*5.5f+100*6.0f+(units-500)*6.5f;
}




