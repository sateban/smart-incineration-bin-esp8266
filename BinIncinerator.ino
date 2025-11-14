#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
// #include <Firebase_ESP_Client.h>
#include <ESP8266mDNS.h>
#include "secrets.h"
#include "max6675.h"

// Provide token process info
// #include "addons/TokenHelper.h"
// #include "addons/RTDBHelper.h"

// ======== SERVER SETUP ========
ESP8266WebServer server(80);

// FirebaseData fbdo;
// FirebaseAuth auth;
// FirebaseConfig config;

// ======== LED PIN ========
// const int LED_PIN = LED_BUILTIN;  // D4 on NodeMCU

// Thermocouple
int thermoSO = D6;   // SO pin
int thermoCS = D5;   // CS pin
int thermoSCK = D7;  // SCK pin
int out = D8;
double celsius;
bool useLocal = true;
// double fahrenheit;

// Gas Sensor
const int mq2Pin = A0;  // Analog pin connected to MQ-2
int gasLevel = 0;       // To store the sensor value
int threshold = 400;    // Adjust this value based on your environment

// Relay for Fan
const int relayCoilPin = D1;
const int relayFanPin = D0;
bool fanStarted = true;

// Timer
String startTimer = "";
unsigned long previousMillis = 0;
unsigned long previousMillis2 = 0;
const long interval = 1000;  // 1 second interval
int seconds = 0;
int seconds2 = 30;

MAX6675 thermocouple(thermoSCK, thermoCS, thermoSO);

// Firebase Stream
// void streamCallback(FirebaseStream data) {
//   Serial.println("🔥 Stream Data Received");

//   if (data.dataType() == "string") {
//     Serial.print("Value: ");
//     Serial.println(data.stringData());
//   }

//   if (data.dataType() == "int") {
//     Serial.print("Value: ");
//     Serial.println(data.intData());
//   }

//   // Add your custom action here
//   // Example:
//   // digitalWrite(relayFanPin, data.intData() == 1 ? LOW : HIGH);
// }

void streamTimeout(bool timeout) {
  if (timeout)
    Serial.println("⚠️ Stream timed out, reconnecting...");
}

// ======== HANDLERS ========
void handleRoot() {
  server.send(200, "text/plain", "Send POST to /led with body 'ON' or 'OFF'");
}

// led
void handleLedControl() {
  if (server.method() == HTTP_POST) {
    String body = server.arg("plain");
    body.trim();

    if (body.equalsIgnoreCase("ON")) {
      // digitalWrite(LED_PIN, LOW);  // Active LOW
      server.send(200, "text/plain", "LED turned ON");

      // if (Firebase.ready()) {
      //   if (Firebase.RTDB.setString(&fbdo, "/settings/message/value", "Hello from ESP8266 (secure)!")) {
      //     Serial.println("✅ Data written successfully!");
      //   }
      //   if (Firebase.RTDB.setTimestamp(&fbdo, "/settings/message")) {
      //     Serial.println("✅ Data written successfully!");
      //   }
      // } else {
      //   Serial.println("⚠️ Firebase not ready yet.");
      // }

    } else if (body.equalsIgnoreCase("OFF")) {
      // digitalWrite(LED_PIN, HIGH);
      server.send(200, "text/plain", "LED turned OFF");
    } else if (body.indexOf("timer") >= 0) {
      body.replace("timer:", "");
      seconds = body.toInt();
      startTimer = "running";
      server.send(200, "text/plain", "Timer sent");
    }
    // Turn on Relay Fan
    else if (body.equalsIgnoreCase("FANON")) {
      digitalWrite(relayFanPin, LOW);
      fanStarted = true;
      server.send(200, "text/plain", "Relay for Fan turned on");
    }
    // Turn off Relay Fan
    else if (body.equalsIgnoreCase("FANOFF")) {
      // digitalWrite(relayFanPin, HIGH);
      server.send(200, "text/plain", "Relay for Fan turned off");
    }
    // Auto
    else if (body.equalsIgnoreCase("AUTo")) {
      // digitalWrite(relayFanPin, LOW);
      server.send(200, "text/plain", "Auto");
    }
    // Use Local Wifi / Firebase
    else if (body.equalsIgnoreCase("CONNL")) {
      // if (Firebase.RTDB.getBool(&fbdo, "/settings/connection/mode")) {
      bool useLocal = true;
      // } else {
      //   Serial.print("❌ Failed to get value: ");
      //   Serial.println(fbdo.errorReason());
      // }
      server.send(200, "text/plain", "LocalWifi");
    } else if (body.equalsIgnoreCase("CONNF")) {
      bool useLocal = false;
      server.send(200, "text/plain", "Firebase");
    }
  } else {
    server.send(405, "text/plain", "Use POST method only.");
  }
}

void handleInformation() {
  if (server.method() == HTTP_POST) {
    String body = server.arg("plain");
    body.trim();
    // bool isFirebase = body == "true";
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

// ✅ /status
void handleStatus() {
  server.send(200, "text/plain", startTimer);
}

void setup() {
  Serial.begin(115200);

  // Thermocouple
  pinMode(out, OUTPUT);
  digitalWrite(out, HIGH);

  // Gas
  // pinMode(mqOut, OUTPUT);
  // digitalWrite(mqOut, HIGH);

  // pinMode(LED_PIN, OUTPUT);
  // digitalWrite(LED_PIN, HIGH);  // off by default

  // Relay Fan
  pinMode(relayFanPin, OUTPUT);
  digitalWrite(relayFanPin, HIGH);

  // Relay Heating Coil
  pinMode(relayCoilPin, OUTPUT);
  digitalWrite(relayCoilPin, HIGH);

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
  // config.api_key = API_KEY;
  // config.database_url = DATABASE_URL;

  // auth.user.email = USER_EMAIL;
  // auth.user.password = USER_PASSWORD;

  // config.token_status_callback = tokenStatusCallback;  // must be set BEFORE begin()
  // Firebase.reconnectWiFi(true);
  // fbdo.setResponseSize(4096);

  // Firebase.begin(&config, &auth);  // initialize after all setup

  // Serial.println("🔥 Firebase initialized");

  // if (Firebase.RTDB.getBool(&fbdo, "/settings/connection/mode")) {
  //   useLocal = !fbdo.boolData();
  // }

  // if (!Firebase.RTDB.beginStream(&fbdo, "/settings/stats/gas")) {
  //   Serial.println("❌ Cannot start stream: " + fbdo.errorReason());
  // }

  // Firebase.RTDB.setStreamCallback(&fbdo, streamCallback, streamTimeout);

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

  if (currentMillis - previousMillis >= interval && startTimer == "running") {
    previousMillis = currentMillis;  // reset timer

    seconds--;
    Serial.print("Timer: ");
    Serial.print(seconds);
    Serial.println(" seconds");
    digitalWrite(relayCoilPin, HIGH);
    digitalWrite(relayFanPin, HIGH);

    // if (!useLocal) {
    //   if (Firebase.ready()) {
    //     if (Firebase.RTDB.setString(&fbdo, "/stats/timer", String(seconds))) {
    //       Serial.println("✅ Timer written successfully!");
    //     }
    //   }
    // }

    if (seconds <= 0) {
      startTimer = "done";
      Serial.println("Time's up!");

      // if (!useLocal) {
      //   if (Firebase.ready()) {
      //     if (Firebase.RTDB.setString(&fbdo, "/stats/timer", "done")) {
      //       Serial.println("✅ Timer done written successfully!");
      //     }
      //   }
      // }

      digitalWrite(relayCoilPin, LOW);
      digitalWrite(relayFanPin, LOW);
    }
  }

  // if (Firebase.RTDB.readStream(&fbdo)) {
  //   if (fbdo.streamAvailable()) {
  //     Serial.println("🔥 Value changed!");

  //     Serial.print("Data path: ");
  //     Serial.println(fbdo.dataPath());

  //     Serial.print("Data type: ");
  //     Serial.println(fbdo.dataType());

  //     if (fbdo.dataType() == "string") {
  //       Serial.print("Value: ");
  //       Serial.println(fbdo.stringData());
  //     }
  //   }
  // }

  // For Fan
  // if (currentMillis - previousMillis2 >= interval && fanStarted) {
  //   previousMillis2 = currentMillis;  // reset timer

  //   seconds2--;
  //   Serial.print("Fan Timer: ");
  //   Serial.print(seconds2);
  //   Serial.println(" seconds");
  //   // digitalWrite(relayCoilPin, HIGH);

  //   if (!useLocal) {
  //     if (Firebase.ready()) {
  //       if (Firebase.RTDB.setString(&fbdo, "/stats/fanTimer", String(seconds))) {
  //         Serial.println("✅ Timer written successfully!");
  //       }
  //     }
  //   }

  //   if (seconds2 <= 0) {
  //     startTimer = "DONEPURIFICATION";
  //     Serial.println("Time's up!");

  //     if (!useLocal) {
  //       if (Firebase.ready()) {
  //         if (Firebase.RTDB.setString(&fbdo, "/stats/fanTimer", "done")) {
  //           Serial.println("✅ Timer done written successfully!");
  //         }
  //       }
  //     }

  //     digitalWrite(relayFanPin, HIGH);

  //     fanStarted = false;
  //   }
  // }

  // --- SENSOR UPDATES EVERY 5 SECONDS ---
  static unsigned long lastSensorUpdate = 0;
  const unsigned long sensorInterval = 5000;  // 5s interval for Firebase writes

  if (currentMillis - lastSensorUpdate >= sensorInterval) {
    lastSensorUpdate = currentMillis;

    float newCelsius = thermocouple.readCelsius();
    int newGasLevel = analogRead(mq2Pin);

    // Only write if values changed significantly
    // if (abs(newCelsius - celsius) >= 0.5) {  // 0.5°C threshold
    celsius = newCelsius;

    // if (!useLocal) {
    //   if (Firebase.ready()) {
    //     Firebase.RTDB.setString(&fbdo, "/stats/temperature", String(celsius));
    //     Serial.println("✅ Temperature updated");
    //   }
    // }
    // }

    // if (abs(newGasLevel - gasLevel) >= 10) {  // 10 ADC threshold
    gasLevel = newGasLevel;
    // if (!useLocal) {
    //   if (Firebase.ready()) {
    //     Firebase.RTDB.setString(&fbdo, "/stats/gas", String(gasLevel));
    //     Serial.println("✅ Gas level updated");
    //   }
    // }
    // }
  }

  // Avoid delay() — use yield() to keep background tasks alive
  yield();
}
