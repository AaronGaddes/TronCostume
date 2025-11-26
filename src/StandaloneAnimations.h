#ifndef STANDALONE_ANIMATIONS_H
#define STANDALONE_ANIMATIONS_H

#include "LEDController.h"
#include <FastLED.h>
#include <Arduino.h>

class StandaloneAnimations
{
public:
  StandaloneAnimations(LEDController* ledController);
  ~StandaloneAnimations();

  // Animation methods
  void off();
  void solid(CRGB color);
  void rainbow();

  // Update method for time-based animations
  void update();

private:
  LEDController* m_ledController;
  unsigned long m_lastUpdateTime;
  uint8_t m_rainbowHue; // Current hue for rainbow animation
};

#endif // STANDALONE_ANIMATIONS_H

