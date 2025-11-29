#ifndef STANDALONE_ANIMATIONS_H
#define STANDALONE_ANIMATIONS_H

#include "LEDController.h"
#include <FastLED.h>
#include <Arduino.h>

class StandaloneAnimations
{
public:
  StandaloneAnimations(LEDController *ledController);
  ~StandaloneAnimations();

  // Animation methods
  void off();
  void solid(CRGB color);
  void rainbow();
  void breathing();
  void chase();
  void twinkle();
  void fire();
  void colorWave();

  // Update method for time-based animations
  void update();

private:
  LEDController *m_ledController;
  unsigned long m_lastUpdateTime;
  uint8_t m_rainbowHue; // Current hue for rainbow animation

  // Animation state variables
  float m_breathingPhase;            // 0.0 to 1.0 for breathing animation
  uint16_t m_chasePosition;          // Position of chase light
  unsigned long m_twinkleLastUpdate; // Last twinkle update time
  uint8_t m_twinkleSparkles[30];     // Sparkle brightness for each LED (max 30 LEDs)
  float m_colorWavePhase;            // Phase for color wave animation
};

#endif // STANDALONE_ANIMATIONS_H
