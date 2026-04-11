#ifndef ANIMATION_MANAGER_H
#define ANIMATION_MANAGER_H

#include "AnimationMode.h"
#include "LEDController.h"
#include "HeartRateAnimation.h"
#include "StandaloneAnimations.h"

class AnimationManager
{
public:
  AnimationManager(LEDController *ledController);
  ~AnimationManager();

  // Initialization
  bool begin();

  // Mode management
  void setMode(AnimationMode mode);
  AnimationMode getCurrentMode() const { return m_currentMode; }
  bool requiresHeartRate() const;

  // Color from BLE / frontend (solid, breathing, chase, twinkle, heart rate pulse)
  void setSolidColor(uint8_t r, uint8_t g, uint8_t b);
  void setSolidColor(uint32_t rgb); // RGB packed as 0xRRGGBB

  // Heart rate pulse: cycle hue through the rainbow instead of solid color
  void setHeartRateRainbowCycle(bool enabled);
  bool getHeartRateRainbowCycle() const { return m_heartRateRainbowCycle; }

  // Heart rate pulse: rainbow spectrum on the traveling wave (mutually exclusive with global cycle in UI)
  void setHeartRateRainbowInPulse(bool enabled);
  bool getHeartRateRainbowInPulse() const { return m_heartRateRainbowInPulse; }

  // Update - call this in main loop
  void update(uint16_t currentHeartRate = 0);

  // Reset current animation
  void reset();

private:
  LEDController *m_ledController;
  HeartRateAnimation *m_heartRateAnimation;
  StandaloneAnimations *m_standaloneAnimations;

  AnimationMode m_currentMode;
  bool m_initialized;
  CRGB m_solidColor; // Current solid color
  bool m_heartRateRainbowCycle;
  bool m_heartRateRainbowInPulse;
};

#endif // ANIMATION_MANAGER_H
