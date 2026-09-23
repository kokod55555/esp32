#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>


const int TRIG_PIN = 5;
const int ECHO_PIN = 18;


const char* WIFI_SSID     = "honor200pro";    
const char* WIFI_PASSWORD = "12345678";  


const char* GOOGLE_SCRIPT_URL = "https://script.google.com/macros/s/AKfycbwz_mb03CHL9gDRrTqIa6FAAm69lV6qc05axZWzwREAPYgoQRr1ZekY-iXBJNHoSOlO/exec";
const char* API_KEY           = "SMARTBOX2026";


const float PACKAGE_THRESHOLD = 20.0; 
bool packageDetected = false;


unsigned long previousSensorMillis = 0;
const unsigned long SENSOR_INTERVAL = 1000;  

unsigned long previousLogMillis = 0;
const unsigned long LOG_INTERVAL = 30000;    


float readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;

  return (duration * 0.0343) / 2.0;
}


void sendToGoogleSheets(float distance, String status, String event) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected!");
    return;
  }

  String json = "{";
  json += "\"apiKey\":\"" + String(API_KEY) + "\",";
  json += "\"distance\":" + String(distance, 2) + ",";
  json += "\"status\":\"" + status + "\",";
  json += "\"event\":\"" + event + "\"";
  json += "}";

  Serial.println("\n--- Sending Data ---");
  Serial.println("Payload: " + json);


  int httpCode = -1;
  String location = "";

  {
    WiFiClientSecure client1;
    client1.setInsecure();

    HTTPClient http;
    http.begin(client1, GOOGLE_SCRIPT_URL);
    http.setTimeout(10000);
    http.addHeader("Content-Type", "application/json");

    const char* headerKeys[] = { "Location" };
    http.collectHeaders(headerKeys, 1);

    httpCode = http.POST(json);
    Serial.print("HTTP Response Code: ");
    Serial.println(httpCode);

    if (httpCode == 301 || httpCode == 302 || httpCode == 307) {
      location = http.header("Location");
      Serial.println("Redirect -> [" + location + "]");
    } else if (httpCode > 0) {
      Serial.println("Response: " + http.getString());
    } else {
      Serial.printf("HTTP Error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
    client1.stop();
  }


  if (location.length() > 0) {
    delay(100);

    WiFiClientSecure client2;
    client2.setInsecure();

    HTTPClient http2;
    http2.begin(client2, location);
    http2.setTimeout(10000);

    int httpCode2 = http2.GET();
    Serial.print("HTTP Response Code (after redirect): ");
    Serial.println(httpCode2);

    if (httpCode2 > 0) {
      Serial.println("Response: " + http2.getString());
    } else {
      Serial.printf("HTTP Error: %s\n", http2.errorToString(httpCode2).c_str());
    }

    http2.end();
    client2.stop();
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.println("\n==================================");
  Serial.println(" SMART PARCEL BOX INITIALIZING ");
  Serial.println("==================================");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  float distance = readUltrasonic();
  if (distance > 0 && distance <= PACKAGE_THRESHOLD) {
    packageDetected = true;
  } else {
    packageDetected = false;
  }

  Serial.print("Initial Status: ");
  Serial.println(packageDetected ? "PACKAGE" : "ว่างเปล่า");
}


void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousSensorMillis >= SENSOR_INTERVAL) {
    previousSensorMillis = currentMillis;

    float distance = readUltrasonic();
    if (distance < 0) return;

    bool newPackageState = (distance <= PACKAGE_THRESHOLD);

    if (newPackageState != packageDetected) {
      packageDetected = newPackageState;

      if (packageDetected) {
        Serial.println("📦 EVENT: PACKAGE IN!");
        sendToGoogleSheets(distance, "ของมาส่ง", "สินค้าเข้า");
      } else {
        Serial.println("📭 EVENT: PACKAGE OUT!");
        sendToGoogleSheets(distance, "ของออก", "สินค้าออก");
      }
    }
  }

  if (currentMillis - previousLogMillis >= LOG_INTERVAL) {
    previousLogMillis = currentMillis;

    float distance = readUltrasonic();
    if (distance > 0) {
      sendToGoogleSheets(distance, packageDetected ? "ของมาส่ง" : "ว่างเปล่า", "");
    }
  }
}