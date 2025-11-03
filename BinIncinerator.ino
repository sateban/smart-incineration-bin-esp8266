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
int out = D8;
double celsius;
double fahrenheit;

// Gas Sensor
const int mq2Pin = D3;  // Analog pin connected to MQ-2
int gasLevel = 0;       // To store the sensor value
int threshold = 400;    // Adjust this value based on your environment

// Relay for Fan
const int relayCoilPin = D0;
const int relayFanPin = D1;

// Timer
bool startTimer = false;
unsigned long previousMillis = 0;
const long interval = 1000;  // 1 second interval
int seconds = 0;

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
    } else if (body.indexOf("timer") >= 0) {
      body.replace("timer:", "");
      seconds = body.toInt();
      startTimer = true;
      server.send(200, "text/plain", "Timer sent");
    }
    // Turn on Relay Fan
    else if (body.equalsIgnoreCase("FANON")) {
      digitalWrite(relayFanPin, HIGH);
      server.send(200, "text/plain", "Relay for Fan turned on");
    }
    // Turn off Relay Fan
    else if (body.equalsIgnoreCase("FANOFF")) {
      digitalWrite(relayFanPin, LOW);
      server.send(200, "text/plain", "Relay for Fan turned off");
    }
    // Auto
    else if (body.equalsIgnoreCase("AUTo")) {
      // digitalWrite(relayFanPin, LOW);
      server.send(200, "text/plain", "Auto");
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
      "temperature:" + String(isnan(temperature) ? 0 : temperature) + ";smoke:" + String(gasLevel));

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

// ✅ Status
void handleStatus() {
  server.send(200, "text/plain", startTimer ? "running" : "done");
}

void setup() {
  Serial.begin(115200);

  // Thermocouple
  pinMode(out, OUTPUT);
  digitalWrite(out, HIGH);

  // Gas
  pinMode(mqOut, OUTPUT);
  digitalWrite(mqOut, HIGH);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // off by default

  // Relay Fan
  pinMode(relayFanPin, OUTPUT);
  digitalWrite(relayFanPin, LOW);

  // Relay Heating Coil
  pinMode(relayCoilPin, OUTPUT);
  digitalWrite(relayCoilPin, LOW);

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
  server.on("/status", HTTP_GET, handleStatus);
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

  // Timer
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval && startTimer) {
    previousMillis = currentMillis;  // reset timer

    seconds--;
    Serial.print("Timer: ");
    Serial.print(seconds);
    Serial.println(" seconds");

    if (seconds <= 0) {
      startTimer = false;
      Serial.println("Time's up!");
    }
  }


  // Read temperature in Celsius
  celsius = thermocouple.readCelsius();
  // Read temperature in Fahrenheit
  fahrenheit = thermocouple.readFahrenheit();

  // Read Gas Value
  gasLevel = analogRead(mq2Pin);
  delay(500);
  yield();
}
