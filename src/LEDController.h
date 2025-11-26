#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <FastLED.h>

// Configuration
#define LED_PIN GPIO_NUM_5
#define LED_COUNT 30
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define MAX_VOLTAGE 5
#define MAX_MILLI_AMPS 1000

class LEDController
{
public:
  LEDController();
  ~LEDController();

  // Initialization
  bool begin(uint16_t ledCount = LED_COUNT);

  // Basic control
  void clear();
  void show();
  void setPixel(uint16_t index, CRGB color);
  void fill(CRGB color);
  void fill(CRGB color, uint16_t first, uint16_t count);

  // Access to LED array
  CRGB *getLEDs() { return m_leds; }
  uint16_t getLEDCount() const { return m_ledCount; }

  // Brightness control
  void setBrightness(uint8_t brightness);
  uint8_t getBrightness() const;

private:
  CRGB *m_leds;
  uint16_t m_ledCount;
  bool m_initialized;
};

#endif // LED_CONTROLLER_H
