// Node settings. Every value can also be set per board with -D in
// platformio.ini, which wins over the defaults here.
#pragma once

// Label shown on the hub. Empty = the hub shows "node XXXX" (low 16 bits of
// the node id), which is unique per board without any setup. Max 12 chars.
#ifndef NODE_NAME
#define NODE_NAME ""
#endif

// Seconds asleep between reports.
#ifndef NODE_SLEEP_SECONDS
#define NODE_SLEEP_SECONDS 60
#endif

#ifndef NODE_I2C_SDA
#define NODE_I2C_SDA 21
#endif
#ifndef NODE_I2C_SCL
#define NODE_I2C_SCL 22
#endif

// ADC1 pin wired to the middle of a divider across the battery, or -1 for
// none (packets then carry battery = 255, "not measured"). ADC2 pins do not
// work while Wi-Fi is on; the node reads before starting Wi-Fi, but stick to
// ADC1 anyway.
#ifndef NODE_BATTERY_PIN
#define NODE_BATTERY_PIN -1
#endif
// Battery voltage / pin voltage. 2.0 for two equal resistors.
#ifndef NODE_BATTERY_DIVIDER
#define NODE_BATTERY_DIVIDER 2.0f
#endif
