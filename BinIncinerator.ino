#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Firebase_ESP_Client.h>

// ======== CONFIGURATION ========
const char* ssid = "ZTE_2.4G_QpEdq4";
const char* password = "mHc7eAkb";

// Firebase configuration
#define API_KEY "AIzaSyB5JnDEZ7lMpiPzkcU5Ht7kWcvs759CkNY"
#define DATABASE_URL "https://smart-incineration-default-rtdb.firebaseio.com/"
#define USER_EMAIL "smartincineration@gmail.com"
#define USER_PASSWORD "smartincineration@auth"

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
  }
  Serial.println("\n✅ WiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // ====== Server Routes ======
  server.on("/", handleRoot);
  server.on("/led", handleLedControl);
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
}

void loop() {
  server.handleClient();
}
