#include <WiFi.h>
#include <PubSubClient.h>

// Wokwi default WiFi. Replace with your own credentials for a real ESP32.
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

const char* MQTT_HOST = "broker.hivemq.com";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_TOPIC = "level3-demo/esp32/readings/kavash-001";

const int SENSOR_PIN = 34;
const unsigned long SAMPLE_MS = 2000;
const unsigned long WIFI_RETRY_MS = 5000;
const unsigned long MQTT_RETRY_MS = 5000;
const unsigned long PUBLISH_RETRY_MS = 500;

const size_t BUFFER_SIZE = 30;

struct Reading {
  unsigned long sequence;
  unsigned long uptimeMs;
  int analogValue;
};

Reading queueData[BUFFER_SIZE];
size_t head = 0, tail = 0, queued = 0;
unsigned long sequenceNumber = 1;
unsigned long lastSample = 0, lastWiFiTry = 0;
unsigned long lastMqttTry = 0, lastPublishTry = 0;
unsigned long droppedReadings = 0;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

void enqueue(const Reading& r) {
  // When full, discard the oldest reading and preserve the newest.
  if (queued == BUFFER_SIZE) {
    head = (head + 1) % BUFFER_SIZE;
    queued--;
    droppedReadings++;
  }
  queueData[tail] = r;
  tail = (tail + 1) % BUFFER_SIZE;
  queued++;
}

void maintainWiFi(unsigned long now) {
  if (WiFi.status() == WL_CONNECTED) return;

  if (now - lastWiFiTry >= WIFI_RETRY_MS) {
    lastWiFiTry = now;
    Serial.println("WiFi reconnect attempt...");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
}

void maintainMqtt(unsigned long now) {
  if (WiFi.status() != WL_CONNECTED || mqtt.connected()) return;

  if (now - lastMqttTry >= MQTT_RETRY_MS) {
    lastMqttTry = now;
    String clientId = "esp32-level3-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    Serial.print("MQTT connecting...");
    if (mqtt.connect(clientId.c_str())) {
      Serial.println(" connected");
    } else {
      Serial.print(" failed, state=");
      Serial.println(mqtt.state());
    }
  }
}

void publishOldest() {
  if (!queued || !mqtt.connected()) return;

  const Reading& r = queueData[head];
  char payload[128];
  snprintf(payload, sizeof(payload),
           "{\"sequence\":%lu,\"uptime_ms\":%lu,\"analog\":%d}",
           r.sequence, r.uptimeMs, r.analogValue);

  if (mqtt.publish(MQTT_TOPIC, payload)) {
    Serial.print("Published: ");
    Serial.println(payload);
    head = (head + 1) % BUFFER_SIZE;
    queued--;
  } else {
    Serial.println("Publish failed; reading kept in queue.");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(SENSOR_PIN, INPUT);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setKeepAlive(20);
  mqtt.setSocketTimeout(2);
  Serial.println("Level 3 MQTT sensor starting");
}

void loop() {
  const unsigned long now = millis();

  maintainWiFi(now);
  maintainMqtt(now);

  if (mqtt.connected()) mqtt.loop();

  // Keep sampling even when WiFi/MQTT is unavailable.
  if (now - lastSample >= SAMPLE_MS) {
    lastSample = now;
    Reading r = {sequenceNumber++, now, analogRead(SENSOR_PIN)};
    enqueue(r);

    Serial.print("Sampled #");
    Serial.print(r.sequence);
    Serial.print(" analog=");
    Serial.print(r.analogValue);
    Serial.print(" buffered=");
    Serial.print(queued);
    Serial.print(" dropped=");
    Serial.println(droppedReadings);
  }

  // Send buffered readings oldest-first after MQTT reconnects.
  if (mqtt.connected() && queued &&
      now - lastPublishTry >= PUBLISH_RETRY_MS) {
    lastPublishTry = now;
    publishOldest();
  }
  // No delay(): the loop continues checking sampling and connection state.
}
