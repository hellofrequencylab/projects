// Hello world for the Adafruit Feather ESP32-S3 TFT:
// draws on the 240x135 display, cycles the NeoPixel, and logs to serial.
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_NeoPixel.h>

Adafruit_ST7789 tft(TFT_CS, TFT_DC, TFT_RST);
Adafruit_NeoPixel pixel(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);

  // Power up the display and the I2C/NeoPixel rail
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);
  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(10);

  tft.init(135, 240);
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(3);
  tft.setCursor(10, 20);
  tft.println("Hello,");
  tft.setTextColor(ST77XX_MAGENTA);
  tft.setCursor(10, 55);
  tft.println("World!");
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 110);
  tft.print("MAC ");
  tft.print(ESP.getEfuseMac(), HEX);

  pixel.begin();
  pixel.setBrightness(30);
}

void loop() {
  static uint16_t hue = 0;
  pixel.setPixelColor(0, pixel.gamma32(pixel.ColorHSV(hue)));
  pixel.show();
  hue += 256;

  static uint32_t last = 0;
  if (millis() - last > 2000) {
    last = millis();
    Serial.printf("alive: %lus, free heap %u\n", millis() / 1000, ESP.getFreeHeap());
  }
  delay(20);
}
