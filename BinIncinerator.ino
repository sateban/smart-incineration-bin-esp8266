#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
// #include <FirebaseESP8266.h>
#include <Firebase_ESP_Client.h>

// ======== CONFIGURATION ========
const char* ssid = "ZTE_2.4G_QpEdq4";
const char* password = "mHc7eAkb";

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#define API_KEY "AIzaSyB5JnDEZ7lMpiPzkcU5Ht7kWcvs759CkNY"           // from Project Settings
#define DATABASE_URL "https://smart-incineration.firebaseapp.com/"  // from Service Accounts

#define USER_EMAIL "smartincineration@gmail.com"
#define USER_PASSWORD "smartincineration@auth"

// ======== SERVER SETUP ========
ESP8266WebServer server(80);

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ======== LED PIN ========
const int LED_PIN = LED_BUILTIN;  // Built-in LED (D4 on NodeMCU)

void handleRoot() {
  server.send(200, "text/plain", "Send POST to /led with body 'ON' or 'OFF'");
}

void handleLedControl() {
  if (server.method() == HTTP_POST) {
    String body = server.arg("plain");  // Read raw POST body
    body.trim();                        // Remove spaces/newlines

    if (body.equalsIgnoreCase("ON")) {
      digitalWrite(LED_PIN, LOW);  // Active LOW on NodeMCU
      server.send(200, "text/plain", "LED turned ON");
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
  digitalWrite(LED_PIN, HIGH);  // LED off by default (active LOW)

  // Connect to Wi-Fi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Server routes
  server.on("/", handleRoot);
  server.on("/led", handleLedControl);

  server.begin();
  Serial.println("HTTP server started");

  // ✅ Firebase
  // Firebase configuration
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Assign user credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Optional: Set logging for debugging
  // Firebase.reconnectWiFi(true);
  // fbdo.setResponseSize(4096);
  // config.token_status_callback = tokenStatusCallback;  // optional (for debugging token info)

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  server.handleClient();
}
