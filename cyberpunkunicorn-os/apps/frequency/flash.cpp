#include "flash.h"

#include "../cyd_pins.h"
#include <Arduino.h>

namespace freq {

namespace {

volatile bool running = false;

void setLed(bool r, bool g, bool b) {
    // Common anode: LOW lights a colour (cyd_pins.h).
    digitalWrite(CYD_LED_R, r == CYD_LED_ACTIVE_LOW ? LOW : HIGH);
    digitalWrite(CYD_LED_G, g == CYD_LED_ACTIVE_LOW ? LOW : HIGH);
    digitalWrite(CYD_LED_B, b == CYD_LED_ACTIVE_LOW ? LOW : HIGH);
}

void flashTask(void *) {
    pinMode(CYD_LED_R, OUTPUT);
    pinMode(CYD_LED_G, OUTPUT);
    pinMode(CYD_LED_B, OUTPUT);
    // Red, yellow, green, cyan, blue, magenta, twice round, then a white blink.
    static const bool COLOURS[][3] = {{1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 1, 1}, {0, 0, 1}, {1, 0, 1}};
    for (int lap = 0; lap < 2; lap++)
        for (const auto &c : COLOURS) {
            setLed(c[0], c[1], c[2]);
            vTaskDelay(pdMS_TO_TICKS(70));
            setLed(false, false, false);
            vTaskDelay(pdMS_TO_TICKS(40));
        }
    for (int i = 0; i < 3; i++) {
        setLed(true, true, true);
        vTaskDelay(pdMS_TO_TICKS(90));
        setLed(false, false, false);
        vTaskDelay(pdMS_TO_TICKS(70));
    }
    // Back to undriven, as the pins were before (nothing else uses them).
    pinMode(CYD_LED_R, INPUT);
    pinMode(CYD_LED_G, INPUT);
    pinMode(CYD_LED_B, INPUT);
    running = false;
    vTaskDelete(nullptr);
}

} // namespace

void flashSuccess() {
    if (running) return;
    running = true;
    if (xTaskCreate(flashTask, "freq_flash", 2048, nullptr, 1, nullptr) != pdPASS) running = false;
}

} // namespace freq
