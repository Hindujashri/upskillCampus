#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

// WiFi credentials (keep placeholder for GitHub)
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// Google Apps Script URL
String scriptURL = "https://script.google.com/macros/s/AKfycbw--x0BNvpwQpcOgichm4m2d38svGQUYEgBr5ELNtzwN6jvLgUcRQOzoT9YwOj4c1CfnA/exec";

// Pin definitions
#define FLOW_SENSOR_PIN 14
#define LED_PIN 25
#define BUZZER_PIN 26

// Patient details
String patientName = "Sample Patient";
String age = "22";
String medicationName = "Normal Saline";
String dosageML = "500";

// Time variables
String dateValue = "";
String startTime = "";
String endTime = "";
String statusValue = "Running";

// Flow variables
volatile int pulseCount = 0;
bool ivCompleted = false;
unsigned long lastFlowTime = 0;
const unsigned long noFlowTimeout = 10000;

// Time setup
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;
const int daylightOffset_sec = 0;

// Flow sensor interrupt
void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

// Get date
String getCurrentDate() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "NA";
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%d-%m-%Y", &timeinfo);
  return String(buffer);
}

// Get time
String getCurrentTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "NA";
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
  return String(buffer);
}

// Send data to Google Sheet
void sendDataToCloud() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = scriptURL +
                 "?patientName=" + patientName +
                 "&age=" + age +
                 "&date=" + dateValue +
                 "&startTime=" + startTime +
                 "&endTime=" + endTime +
                 "&medicationName=" + medicationName +
                 "&dosageML=" + dosageML +
                 "&status=" + statusValue;

    url.replace(" ", "%20");

    http.begin(url);
    http.GET();
    http.end();
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), pulseCounter, FALLING);

  // Connect WiFi
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  // Get internet time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  delay(2000);

  dateValue = getCurrentDate();
  startTime = getCurrentTime();

  lastFlowTime = millis();
}

void loop() {
  if (ivCompleted) return;

  // Flow detected
  if (pulseCount > 0) {
    lastFlowTime = millis();
    pulseCount = 0;
  }

  // No flow means IV completed
  if ((millis() - lastFlowTime) > noFlowTimeout) {
    ivCompleted = true;
    statusValue = "IV Completed";
    endTime = getCurrentTime();

    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);

    sendDataToCloud();

    delay(5000);

    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }
}