/*
  ESP32 IoT Robot Simulation (Wokwi)
  - Ultrasonic (HC-SR04) + 5 IR switches
  - LEDs simulate motors
  - Wi-Fi + MQTT:
      publishes telemetry to: robot/<id>/telemetry
      receives commands on:  robot/<id>/cmd
  Commands (payload): F, B, L, R, S
*/

#include <WiFi.h>
#include <PubSubClient.h>

// ---------- WiFi (Wokwi) ----------
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

// ---------- MQTT Broker ----------
const char* MQTT_HOST = "broker.hivemq.com";
const int   MQTT_PORT = 1883;

// Make this UNIQUE to avoid clashes
String ROBOT_ID = "robot01_fix_123";
String TOPIC_TELE = "robot/" + ROBOT_ID + "/telemetry";
String TOPIC_CMD  = "robot/" + ROBOT_ID + "/cmd";

WiFiClient espClient;
PubSubClient mqtt(espClient);

// ---------- Pins ----------
#define TRIG 5
#define ECHO 18

int irPins[5] = {32, 33, 25, 26, 27};

// LEDs simulate motors
int motorLeft  = 14;
int motorRight = 15;

// ---------- Timing ----------
unsigned long lastTeleMs = 0;
const unsigned long TELE_PERIOD_MS = 1000;

unsigned long lastCmdMs = 0;
const unsigned long CMD_TIMEOUT_MS = 3000; // stop if no command for 3s

// ---------- Ultrasonic ----------
float readDistanceCM() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 25000); // 25ms timeout
  if (duration == 0) return -1;               // invalid/no echo
  return (duration * 0.0343f) / 2.0f;
}

// ---------- Motor (LED) helpers ----------
void motorsStop() {
  digitalWrite(motorLeft, LOW);
  digitalWrite(motorRight, LOW);
}
void motorsForward() {
  digitalWrite(motorLeft, HIGH);
  digitalWrite(motorRight, HIGH);
}
void motorsLeft() {
  digitalWrite(motorLeft, LOW);
  digitalWrite(motorRight, HIGH);
}
void motorsRight() {
  digitalWrite(motorLeft, HIGH);
  digitalWrite(motorRight, LOW);
}
void motorsBackwardIndicator() {
  // LEDs can't show reverse direction well; blink both as an indicator
  digitalWrite(motorLeft, HIGH);
  digitalWrite(motorRight, HIGH);
  delay(150);
  motorsStop();
  delay(150);
}

// ---------- MQTT callback (dashboard -> robot) ----------
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();

  lastCmdMs = millis();

  Serial.print("CMD received: ");
  Serial.println(msg);

  char c = msg.length() > 0 ? msg.charAt(0) : 'S';
  switch (c) {
    case 'F': motorsForward(); break;
    case 'L': motorsLeft(); break;
    case 'R': motorsRight(); break;
    case 'B': motorsBackwardIndicator(); break;
    case 'S':
    default:  motorsStop(); break;
  }
}

// ---------- Connect helpers ----------
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqttMessage);

  Serial.print("Connecting MQTT");
  while (!mqtt.connected()) {
    String clientId = "wokwi-" + ROBOT_ID + "-" + String(random(0xffff), HEX);
    if (mqtt.connect(clientId.c_str())) {
      Serial.println("\nMQTT connected.");
      mqtt.subscribe(TOPIC_CMD.c_str());
      Serial.print("Subscribed: ");
      Serial.println(TOPIC_CMD);
    } else {
      Serial.print(".");
      delay(600);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  // Switch wiring: COM->GPIO, side->GND => use INPUT_PULLUP
  for (int i = 0; i < 5; i++) {
    pinMode(irPins[i], INPUT_PULLUP);
  }

  pinMode(motorLeft, OUTPUT);
  pinMode(motorRight, OUTPUT);
  motorsStop();

  connectWiFi();
  connectMQTT();

  lastCmdMs = millis();
  Serial.println("ESP32 IoT Robot Simulation Started");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  // Safety: stop if no recent command
  if (millis() - lastCmdMs > CMD_TIMEOUT_MS) {
    motorsStop();
  }

  // Telemetry publish
  if (millis() - lastTeleMs >= TELE_PERIOD_MS) {
    lastTeleMs = millis();

    float dist = readDistanceCM();

    int ir[5];
    for (int i = 0; i < 5; i++) ir[i] = digitalRead(irPins[i]);

    // JSON telemetry
    String json = "{";
    json += "\"dist_cm\":" + String(dist, 2) + ",";
    json += "\"ir\":[" + String(ir[0]) + "," + String(ir[1]) + "," + String(ir[2]) + "," + String(ir[3]) + "," + String(ir[4]) + "],";
    json += "\"rssi\":" + String(WiFi.RSSI());
    json += "}";

    mqtt.publish(TOPIC_TELE.c_str(), json.c_str());

    Serial.print("PUB -> ");
    Serial.println(json);

    // Local safety demo: if obstacle too close, stop (independent of remote control)
    if (dist > 0 && dist < 20) {
      motorsStop();
      Serial.println("SAFETY: Obstacle too close -> STOP");
    }
  }
}
