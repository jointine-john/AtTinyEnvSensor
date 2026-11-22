// ESP32 receiver: decodes 433 MHz packets from the ATtiny sensor nodes and
// publishes them to MQTT with Home Assistant auto-discovery. Each node's
// readings (temperature, humidity, pressure, wind, rain...) are announced to
// Home Assistant the first time the node sends them.
//
// Wiring: RXB6 VCC -> 3V3, GND -> GND, DATA -> GPIO4, 17.3 cm wire antenna.
// SRX882: same, plus CS -> 3V3 (see docs/wiring.md).
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "soc/soc.h"
#include "soc/gpio_reg.h"

#include "decoder.h"
#include "packet.h"
#include "secrets.h"
#ifdef LOOPBACK_TEST
#include "loopback.h"
#endif

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

// One Home Assistant entity per reading. `bit` is the SensorPacket::present
// bit, or BATTERY for the header's battery voltage.
struct Entity {
    uint16_t bit;
    const char *key;            // JSON key in the state payload
    const char *name;
    const char *deviceClass;    // nullptr for none
    const char *unit;
    const char *stateClass;
};

constexpr uint16_t BATTERY = 1 << 15;

static const Entity ENTITIES[] = {
    {SensorPacket::TEMPERATURE, "temperature", "Temperature", "temperature", "\xC2\xB0" "C", "measurement"},
    {SensorPacket::HUMIDITY, "humidity", "Humidity", "humidity", "%", "measurement"},
    {SensorPacket::PRESSURE, "pressure", "Pressure", "atmospheric_pressure", "hPa", "measurement"},
    {SensorPacket::WIND_SPEED, "wind_speed", "Wind speed", "wind_speed", "m/s", "measurement"},
    {SensorPacket::WIND_GUST, "wind_gust", "Wind gust", "wind_speed", "m/s", "measurement"},
    {SensorPacket::WIND_DIRECTION, "wind_direction", "Wind direction", nullptr, "\xC2\xB0", "measurement"},
    {SensorPacket::RAIN, "rain", "Rain", "precipitation", "mm", "total_increasing"},
    {BATTERY, "battery", "Battery", "voltage", "V", "measurement"},
};

struct NodeState {
    bool seen = false;
    uint16_t announced = 0;     // Entity bits already sent to Home Assistant
    uint8_t lastSeq = 0;
    uint32_t lastMs = 0;
};
static NodeState nodes[256];

static void publishDiscovery(uint8_t id, const Entity &e)
{
    char topic[96];
    char payload[640];
    char deviceClass[48] = "";
    if (e.deviceClass)
        snprintf(deviceClass, sizeof(deviceClass), "\"device_class\":\"%s\",", e.deviceClass);
    snprintf(topic, sizeof(topic), "homeassistant/sensor/attiny_th_%u/%s/config", id, e.key);
    snprintf(payload, sizeof(payload),
             "{\"name\":\"%s\",\"unique_id\":\"attiny_th_%u_%s\","
             "\"state_topic\":\"attiny_th/%u/state\","
             "\"value_template\":\"{{ value_json.%s }}\","
             "%s\"unit_of_measurement\":\"%s\","
             "\"state_class\":\"%s\",\"expire_after\":%lu,"
             "\"device\":{\"identifiers\":[\"attiny_th_%u\"],"
             "\"name\":\"ATtiny sensor %u\",\"model\":\"ATtiny sensor node\","
             "\"manufacturer\":\"DIY\"}}",
             e.name, id, e.key, id, e.key, deviceClass, e.unit, e.stateClass,
             (unsigned long)EXPIRE_AFTER_S, id, id);
    mqtt.publish(topic, payload, true);
}

// Announces any of this packet's readings that Home Assistant hasn't seen yet.
static void announce(const SensorPacket &sp)
{
    uint16_t have = sp.present | (sp.batteryMv ? BATTERY : 0);
    NodeState &n = nodes[sp.node];
    for (const Entity &e : ENTITIES) {
        if ((have & e.bit) && !(n.announced & e.bit)) {
            publishDiscovery(sp.node, e);
            n.announced |= e.bit;
        }
    }
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
            n.announced = 0;    // re-announce in case the broker lost retained config
    } else {
        Serial.printf("MQTT connect failed, rc=%d\n", mqtt.state());
    }
}

// --- Packet handling ----------------------------------------------------------

// Appends `,"key":value` to the JSON being built in buf.
static void addField(char *buf, size_t size, size_t &len, const char *key, const char *fmt, double v)
{
    if (len >= size)
        return;
    len += snprintf(buf + len, size - len, ",\"%s\":", key);
    if (len < size)
        len += snprintf(buf + len, size - len, fmt, v);
}

static void handlePacket(const uint8_t *pkt, uint8_t pktLen)
{
    SensorPacket sp;
    if (!parsePacket(pkt, pktLen, sp)) {
        Serial.println("malformed packet (valid CRC), ignored");
        return;
    }

    NodeState &n = nodes[sp.node];
    uint32_t now = millis();
    if (n.seen && n.lastSeq == sp.seq && now - n.lastMs < DEDUP_WINDOW_MS)
        return;     // repeat of a packet we already have
    n.seen = true;
    n.lastSeq = sp.seq;
    n.lastMs = now;

    // Build the state payload and the log line together.
    char payload[320];
    size_t len = snprintf(payload, sizeof(payload), "{\"seq\":%u", sp.seq);
    if (sp.has(SensorPacket::TEMPERATURE))
        addField(payload, sizeof(payload), len, "temperature", "%.2f", sp.temperature / 100.0);
    if (sp.has(SensorPacket::HUMIDITY))
        addField(payload, sizeof(payload), len, "humidity", "%.2f", sp.humidity / 100.0);
    if (sp.has(SensorPacket::PRESSURE))
        addField(payload, sizeof(payload), len, "pressure", "%.1f", sp.pressure / 10.0);
    if (sp.has(SensorPacket::WIND_SPEED))
        addField(payload, sizeof(payload), len, "wind_speed", "%.2f", sp.windSpeed / 100.0);
    if (sp.has(SensorPacket::WIND_GUST))
        addField(payload, sizeof(payload), len, "wind_gust", "%.2f", sp.windGust / 100.0);
    if (sp.has(SensorPacket::WIND_DIRECTION))
        addField(payload, sizeof(payload), len, "wind_direction", "%.1f", sp.windDirection / 10.0);
    if (sp.has(SensorPacket::RAIN))
        addField(payload, sizeof(payload), len, "rain", "%.2f",
                 (double)sp.rainTips * sp.rainUmPerTip / 1000.0);
    if (sp.batteryMv)
        addField(payload, sizeof(payload), len, "battery", "%.3f", sp.batteryMv / 1000.0);
    if (len < sizeof(payload) - 1) {
        payload[len++] = '}';
        payload[len] = '\0';
    }

    Serial.printf("node %u: %s%s\n", sp.node, payload,
                  (sp.status & PROTO_STATUS_SENSOR_ERROR) ? "  SENSOR ERROR" : "");
    if (sp.unknown)
        Serial.printf("node %u: %u reading(s) of unknown type skipped\n", sp.node, sp.unknown);

    if (!mqtt.connected())
        return;
    announce(sp);

    char topic[32];
    snprintf(topic, sizeof(topic), "attiny_th/%u/state", sp.node);
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

#ifdef LOOPBACK_TEST
    loopbackBegin();
#endif
}

void loop()
{
    while (ringTail != ringHead) {
        Pulse pulse = ring[ringTail];
        ringTail = (ringTail + 1) % RING_SIZE;
        if (decoder.feedPulse(pulse.level, pulse.durationUs))
            handlePacket(decoder.packet(), decoder.packetLen());
    }

    ensureConnected();
    mqtt.loop();

#ifdef LOOPBACK_TEST
    loopbackLoop();
#endif
}
