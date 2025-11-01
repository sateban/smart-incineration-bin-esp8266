#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Firebase_ESP_Client.h>
#include <ESP8266mDNS.h>
#include "secrets.h"
#include "max6675.h"

// Provide token process info
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ======== SERVER SETUP ========
ESP8266WebServer server(80);

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ======== LED PIN ========
const int LED_PIN = LED_BUILTIN;  // D4 on NodeMCU

// Thermocouple
int thermoSO = D6;   // SO pin
int thermoCS = D5;   // CS pin
int thermoSCK = D7;  // SCK pin
double celsius;
double fahrenheit;

// Gas Sensor
const int mq2Pin = D0;  // Analog pin connected to MQ-2
int gasLevel = 0;       // To store the sensor value
int threshold = 400;    // Adjust this value based on your environment

MAX6675 thermocouple(thermoSCK, thermoCS, thermoSO);

// ======== HANDLERS ========
void handleRoot() {
  server.send(200, "text/plain", "Send POST to /led with body 'ON' or 'OFF'");
}

void handleLedControl() {
  if (server.method() == HTTP_POST) {
    String body = server.arg("plain");
    body.trim();

    if (body.equalsIgnoreCase("ON")) {
      digitalWrite(LED_PIN, LOW);  // Active LOW
      server.send(200, "text/plain", "LED turned ON");

      if (Firebase.ready()) {
        if (Firebase.RTDB.setString(&fbdo, "/test/message", "Hello from ESP8266 (secure)!")) {
          Serial.println("✅ Data written successfully!");
        } else {
          Serial.println("❌ Write failed: " + fbdo.errorReason());
        }
      } else {
        Serial.println("⚠️ Firebase not ready yet.");
      }

    } else if (body.equalsIgnoreCase("OFF")) {
      digitalWrite(LED_PIN, HIGH);
      server.send(200, "text/plain", "LED turned OFF");
    } else {
      server.send(400, "text/plain", "Invalid command. Use 'ON' or 'OFF'.");
    }
  } else {
    server.send(405, "text/plain", "Use POST method only.");
  }
}

void handleInformation() {
  if (server.method() == HTTP_POST) {
    String body = server.arg("plain");
    body.trim();
    double temperature = celsius;

    server.send(
      200,
      "text/plain",
      "temperature:" + String(isnan(temperature) ? 0 : temperature) + ";smoke:" + String(gasValue));

    // if (Firebase.ready()) {
    //   if (Firebase.RTDB.setString(&fbdo, "/test/message", "Hello from ESP8266 (secure)!")) {
    //     Serial.println("✅ Data written successfully!");
    //   } else {
    //     Serial.println("❌ Write failed: " + fbdo.errorReason());
    //   }
    // } else {
    //   Serial.println("⚠️ Firebase not ready yet.");
    // }

  } else {
    server.send(405, "text/plain", "Use POST method only.");
  }
}


void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // off by default

  // ====== WiFi Setup ======
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    yield();
  }
  Serial.println("\n✅ WiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // ====== Server Routes ======
  server.on("/", handleRoot);
  server.on("/led", handleLedControl);
  server.on("/info", handleInformation);
  server.begin();
  Serial.println("🌐 HTTP server started");

  // ====== Firebase Setup ======
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  config.token_status_callback = tokenStatusCallback;  // must be set BEFORE begin()
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  Firebase.begin(&config, &auth);  // initialize after all setup

  Serial.println("🔥 Firebase initialized");

  // Start mDNS responder with device name "esp8266-device"
  if (MDNS.begin("esp8266-device")) {
    Serial.println("mDNS responder started");
    Serial.println("You can now connect to:");
    Serial.println("http://esp8266-device.local/");
  } else {
    Serial.println("Error starting mDNS");
  }

  // Smoke sensor
  pinMode(mq2Pin, INPUT);  // MQ-2 analog input
  Serial.println("MQ-2 Gas Sensor is warming up...");
  delay(2000);  // Warm-up time for sensor
}

void loop() {
  server.handleClient();
  MDNS.update();  // Required to keep mDNS running

  // Read temperature in Celsius
  celsius = thermocouple.readCelsius();
  // Read temperature in Fahrenheit
  fahrenheit = thermocouple.readFahrenheit();

  // Read Gas Value
  gasLevel = analogRead(mq2Pin);
  delay(1000);
  yield();
}
