#include "packet.h"
#include <string.h>

static uint16_t be16(const uint8_t *p)
{
    return (uint16_t)((p[0] << 8) | p[1]);
}

bool parsePacket(const uint8_t *pkt, uint8_t len, SensorPacket &out)
{
    memset(&out, 0, sizeof(out));
    if (len < PROTO_HEADER_LEN + 2 || pkt[0] != len - 2)
        return false;

    const uint8_t *body = pkt + 1;
    const uint8_t bodyLen = pkt[0];
    out.node = body[0];
    out.seq = body[1];
    out.batteryMv = be16(body + 2);

    uint8_t i = PROTO_HEADER_LEN;
    while (i < bodyLen) {
        uint8_t type = body[i++];
        uint8_t size = proto_value_size(type);
        if (size == 0 || i + size > bodyLen)
            return false;
        const uint8_t *v = body + i;
        i += size;

        switch (type) {
        case PROTO_STATUS:
            out.status = v[0];
            break;
        case PROTO_TEMPERATURE:
            out.temperature = (int16_t)be16(v);
            out.present |= SensorPacket::TEMPERATURE;
            break;
        case PROTO_HUMIDITY:
            out.humidity = be16(v);
            out.present |= SensorPacket::HUMIDITY;
            break;
        case PROTO_PRESSURE:
            out.pressure = be16(v);
            out.present |= SensorPacket::PRESSURE;
            break;
        case PROTO_WIND_SPEED:
            out.windSpeed = be16(v);
            out.present |= SensorPacket::WIND_SPEED;
            break;
        case PROTO_WIND_GUST:
            out.windGust = be16(v);
            out.present |= SensorPacket::WIND_GUST;
            break;
        case PROTO_WIND_DIRECTION:
            out.windDirection = be16(v);
            out.present |= SensorPacket::WIND_DIRECTION;
            break;
        case PROTO_RAIN:
            out.rainTips = be16(v);
            out.rainUmPerTip = be16(v + 2);
            out.present |= SensorPacket::RAIN;
            break;
        default:
            out.unknown++;
            break;
        }
    }
    return true;
}
