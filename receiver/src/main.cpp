// ESP32 receiver: decodes 433 MHz packets from the ATtiny85 sensor nodes and
// publishes them to MQTT with Home Assistant auto-discovery.
//
// Wiring: RXB6 VCC -> 3V3, GND -> GND, DATA -> GPIO4, 17.3 cm wire antenna.
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "soc/soc.h"
#include "soc/gpio_reg.h"

#include "decoder.h"
#include "secrets.h"

constexpr uint8_t RX_PIN = 4;
constexpr uint32_t DEDUP_WINDOW_MS = 5000;
// Sensors report every ~5 min; mark them unavailable after 20 min of silence.
constexpr uint32_t EXPIRE_AFTER_S = 1200;

// --- Edge capture -----------------------------------------------------------

struct Pulse {
    uint32_t durationUs;
    uint8_t level;
};

constexpr uint16_t RING_SIZE = 512;
static Pulse ring[RING_SIZE];
static volatile uint16_t ringHead = 0;
static volatile uint16_t ringTail = 0;
static volatile uint32_t lastEdgeUs = 0;

static void IRAM_ATTR onEdge()
{
    uint32_t now = micros();
    uint8_t levelNow = (REG_READ(GPIO_IN_REG) >> RX_PIN) & 1;
    uint16_t next = (ringHead + 1) % RING_SIZE;
    if (next != ringTail) {
        ring[ringHead].durationUs = now - lastEdgeUs;
        ring[ringHead].level = !levelNow;   // level of the pulse that just ended
        ringHead = next;
    }
    lastEdgeUs = now;
}

// --- MQTT ---------------------------------------------------------------------

static WiFiClient wifiClient;
static PubSubClient mqtt(wifiClient);

struct NodeState {
    bool seen = false;
    bool announced = false;
    uint8_t lastSeq = 0;
    uint32_t lastMs = 0;
};
static NodeState nodes[256];

static void publishDiscovery(uint8_t id, const char *key, const char *name,
                             const char *deviceClass, const char *unit)
{
    char topic[96];
    char payload[640];
    snprintf(topic, sizeof(topic), "homeassistant/sensor/attiny_th_%u/%s/config", id, key);
    snprintf(payload, sizeof(payload),
             "{\"name\":\"%s\",\"unique_id\":\"attiny_th_%u_%s\","
             "\"state_topic\":\"attiny_th/%u/state\","
             "\"value_template\":\"{{ value_json.%s }}\","
             "\"device_class\":\"%s\",\"unit_of_measurement\":\"%s\","
             "\"state_class\":\"measurement\",\"expire_after\":%lu,"
             "\"device\":{\"identifiers\":[\"attiny_th_%u\"],"
             "\"name\":\"ATtiny sensor %u\",\"model\":\"ATtiny85 + SHT40\","
             "\"manufacturer\":\"DIY\"}}",
             name, id, key, id, key, deviceClass, unit,
             (unsigned long)EXPIRE_AFTER_S, id, id);
    mqtt.publish(topic, payload, true);
}

static void announce(uint8_t id)
{
    publishDiscovery(id, "temperature", "Temperature", "temperature", "\xC2\xB0" "C");
    publishDiscovery(id, "humidity", "Humidity", "humidity", "%");
    publishDiscovery(id, "battery", "Battery", "voltage", "V");
    nodes[id].announced = true;
}

static void ensureConnected()
{
    if (WiFi.status() != WL_CONNECTED || mqtt.connected())
        return;

    static uint32_t lastAttempt = 0;
    if (lastAttempt && millis() - lastAttempt < 5000)
        return;
    lastAttempt = millis();

    const char *user = strlen(MQTT_USER) ? MQTT_USER : nullptr;
    const char *pass = strlen(MQTT_PASS) ? MQTT_PASS : nullptr;
    if (mqtt.connect("attiny-th-receiver", user, pass)) {
        Serial.println("MQTT connected");
        for (auto &n : nodes)
            n.announced = false;    // re-announce in case the broker lost retained config
    } else {
        Serial.printf("MQTT connect failed, rc=%d\n", mqtt.state());
    }
}

// --- Packet handling ----------------------------------------------------------

static void handlePacket(const uint8_t *p)
{
    uint8_t id = p[0];
    uint8_t seq = p[1];
    int16_t temp = (int16_t)((p[2] << 8) | p[3]);
    uint16_t rh = (p[4] << 8) | p[5];
    uint16_t vbat = (p[6] << 8) | p[7];

    NodeState &n = nodes[id];
    uint32_t now = millis();
    if (n.seen && n.lastSeq == seq && now - n.lastMs < DEDUP_WINDOW_MS)
        return;     // repeat of a packet we already have
    n.seen = true;
    n.lastSeq = seq;
    n.lastMs = now;

    if (temp == PROTO_TEMP_INVALID) {
        Serial.printf("node %u seq %u: SENSOR ERROR, battery %u mV\n", id, seq, vbat);
        return;
    }

    Serial.printf("node %u seq %u: %.2f C, %.2f %%RH, %u mV\n",
                  id, seq, temp / 100.0, rh / 100.0, vbat);

    if (!mqtt.connected())
        return;
    if (!n.announced)
        announce(id);

    char topic[32];
    char payload[128];
    snprintf(topic, sizeof(topic), "attiny_th/%u/state", id);
    snprintf(payload, sizeof(payload),
             "{\"temperature\":%.2f,\"humidity\":%.2f,\"battery\":%.3f,\"seq\":%u}",
             temp / 100.0, rh / 100.0, vbat / 1000.0, seq);
    mqtt.publish(topic, payload);
}

// --- Main ---------------------------------------------------------------------

static ManchesterDecoder decoder;

void setup()
{
    Serial.begin(115200);
    Serial.println("\nATtiny T/H receiver");

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setBufferSize(768);

    pinMode(RX_PIN, INPUT);
    lastEdgeUs = micros();
    attachInterrupt(digitalPinToInterrupt(RX_PIN), onEdge, CHANGE);
}

void loop()
{
    while (ringTail != ringHead) {
        Pulse pulse = ring[ringTail];
        ringTail = (ringTail + 1) % RING_SIZE;
        if (decoder.feedPulse(pulse.level, pulse.durationUs))
            handlePacket(decoder.packet());
    }

    ensureConnected();
    mqtt.loop();
}
