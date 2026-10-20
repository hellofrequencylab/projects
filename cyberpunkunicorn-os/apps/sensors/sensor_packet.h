/*
 * CyberUnicorn sensor packet, v1: the wire format between a sensor node and the
 * CyberUnicorn 5000 hub, sent over ESP-NOW. See PROTOCOL.md next to this file.
 *
 * This header is the contract. It is plain C99/C++11 with no Arduino or ESP-IDF
 * dependency, so a node project can copy it in unchanged. Do not edit a copy:
 * change it here, bump CU_SENSOR_VERSION if the change is incompatible, and copy
 * it out again.
 *
 * One packet = one reading. A node with several sensors (say a BME280: temperature,
 * humidity, pressure) sends one packet per reading, all with the same node_id.
 * All multi-byte fields are little-endian (every ESP32/ESP8266/RP2040/nRF52 is).
 */
#ifndef CU_SENSOR_PACKET_H
#define CU_SENSOR_PACKET_H

#include <stdint.h>
#include <string.h>

#define CU_SENSOR_MAGIC 0x5543u /* bytes 0x43 0x55 on the wire: "CU" */
#define CU_SENSOR_VERSION 1u
#define CU_SENSOR_NAME_LEN 12   /* label, NUL-padded; need not be NUL-terminated if all 12 used */
#define CU_SENSOR_CHANNEL 1     /* Wi-Fi channel the hub listens on when it isn't joined to Wi-Fi */
#define CU_SENSOR_BATTERY_NONE 255u /* battery field: mains powered / not measured */

/* What was measured. The hub groups and labels readings by this. */
enum cu_sensor_type {
    CU_TYPE_GENERIC = 0,     /* anything else; give it a name */
    CU_TYPE_TEMPERATURE = 1, /* degrees C */
    CU_TYPE_HUMIDITY = 2,    /* % relative humidity */
    CU_TYPE_PRESSURE = 3,    /* hPa (the hub's Altimeter can use it) */
    CU_TYPE_LIGHT = 4,       /* lux */
    CU_TYPE_CO2 = 5,         /* ppm (eCO2 or true CO2) */
    CU_TYPE_TVOC = 6,        /* ppb */
    CU_TYPE_VOLTAGE = 7,     /* V */
    CU_TYPE_CURRENT = 8,     /* mA */
    CU_TYPE_POWER = 9,       /* mW */
    CU_TYPE_ACCEL = 10,      /* g */
    CU_TYPE_GYRO = 11,       /* deg/s */
    CU_TYPE_ANGLE = 12,      /* degrees (tilt, heading) */
    CU_TYPE_ALTITUDE = 13,   /* m */
    CU_TYPE_DISTANCE = 14,   /* cm */
    CU_TYPE_SOIL = 15,       /* % moisture */
    CU_TYPE_MOTION = 16,     /* 0/1 */
    CU_TYPE_COUNT = 17,      /* events, pulses */
    CU_TYPE_LAST = CU_TYPE_COUNT
};

/* Unit of `value`. Usually implied by the type, but sent so a generic reading
 * still displays correctly. */
enum cu_sensor_unit {
    CU_UNIT_NONE = 0,
    CU_UNIT_CELSIUS = 1,
    CU_UNIT_PERCENT = 2,
    CU_UNIT_HPA = 3,
    CU_UNIT_LUX = 4,
    CU_UNIT_PPM = 5,
    CU_UNIT_PPB = 6,
    CU_UNIT_VOLT = 7,
    CU_UNIT_MILLIAMP = 8,
    CU_UNIT_MILLIWATT = 9,
    CU_UNIT_G = 10,
    CU_UNIT_DEG_PER_S = 11,
    CU_UNIT_DEGREE = 12,
    CU_UNIT_METER = 13,
    CU_UNIT_CENTIMETER = 14,
    CU_UNIT_LAST = CU_UNIT_CENTIMETER
};

#if defined(_MSC_VER)
#pragma pack(push, 1)
#define CU_PACKED
#else
#define CU_PACKED __attribute__((packed))
#endif

/* 28 bytes. Field offsets are fixed; every field is naturally aligned anyway. */
typedef struct CU_PACKED cu_sensor_packet {
    uint16_t magic;                 /*  0  CU_SENSOR_MAGIC */
    uint8_t version;                /*  2  CU_SENSOR_VERSION */
    uint8_t type;                   /*  3  enum cu_sensor_type */
    uint32_t node_id;               /*  4  unique per node; default: low 4 bytes of its MAC */
    uint16_t seq;                   /*  8  +1 per packet sent (wraps); the hub counts gaps */
    uint8_t battery;                /* 10  0-100 %, or CU_SENSOR_BATTERY_NONE */
    uint8_t unit;                   /* 11  enum cu_sensor_unit */
    float value;                    /* 12  IEEE-754 single */
    char name[CU_SENSOR_NAME_LEN];  /* 16  e.g. "garden", "attic"; may be empty */
} cu_sensor_packet_t;

#if defined(_MSC_VER)
#pragma pack(pop)
#endif

#ifdef __cplusplus
static_assert(sizeof(cu_sensor_packet_t) == 28, "cu_sensor_packet_t must be 28 bytes");
#else
typedef char cu_sensor_packet_size_check[(sizeof(cu_sensor_packet_t) == 28) ? 1 : -1];
#endif

/* Fill a packet. `name` may be NULL. The caller bumps `seq` per packet. */
static inline void cu_sensor_packet_init(
    cu_sensor_packet_t *p, uint32_t node_id, uint16_t seq, uint8_t type, uint8_t unit, float value,
    uint8_t battery, const char *name
) {
    memset(p, 0, sizeof(*p));
    p->magic = CU_SENSOR_MAGIC;
    p->version = CU_SENSOR_VERSION;
    p->type = type;
    p->node_id = node_id;
    p->seq = seq;
    p->battery = battery;
    p->unit = unit;
    p->value = value;
    if (name) strncpy(p->name, name, CU_SENSOR_NAME_LEN);
}

/* Receiver check: right magic and version, and long enough. Longer packets are
 * accepted (v1 may grow at the end); shorter ones are not. */
static inline int cu_sensor_packet_valid(const uint8_t *data, int len) {
    const cu_sensor_packet_t *p = (const cu_sensor_packet_t *)data;
    if (len < (int)sizeof(cu_sensor_packet_t)) return 0;
    return p->magic == CU_SENSOR_MAGIC && p->version == CU_SENSOR_VERSION;
}

#endif /* CU_SENSOR_PACKET_H */
