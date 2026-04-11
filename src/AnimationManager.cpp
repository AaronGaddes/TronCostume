#include "AnimationManager.h"
#include <Arduino.h>

// Larger value = slower rainbow sweep for heart rate pulse (~82 ms per hue step ≈ 21 s per cycle)
#define HR_RAINBOW_MS_PER_HUE_STEP 82

AnimationManager::AnimationManager(LEDController *ledController)
    : m_ledController(ledController),
      m_heartRateAnimation(nullptr),
      m_standaloneAnimations(nullptr),
      m_currentMode(MODE_OFF),
      m_initialized(false),
      m_solidColor(CRGB::White), // Default to white
      m_heartRateRainbowCycle(false),
      m_heartRateRainbowInPulse(false)
{
}

AnimationManager::~AnimationManager()
{
  if (m_heartRateAnimation != nullptr)
  {
    delete m_heartRateAnimation;
  }
  if (m_standaloneAnimations != nullptr)
  {
    delete m_standaloneAnimations;
  }
}

bool AnimationManager::begin()
{
  if (m_ledController == nullptr)
  {
    Serial.println("ERROR: LEDController is null");
    return false;
  }

  // Create animation objects
  m_heartRateAnimation = new HeartRateAnimation(m_ledController);
  m_standaloneAnimations = new StandaloneAnimations(m_ledController);

  if (m_heartRateAnimation == nullptr || m_standaloneAnimations == nullptr)
  {
    Serial.println("ERROR: Failed to create animation objects");
    return false;
  }

  // Start with OFF mode
  setMode(MODE_OFF);

  m_initialized = true;
  return true;
}

void AnimationManager::setMode(AnimationMode mode)
{
  if (mode < 0 || mode >= MODE_COUNT)
  {
    Serial.print("ERROR: Invalid mode: ");
    Serial.println((int)mode);
    return;
  }

  // Reset previous mode
  reset();

  m_currentMode = mode;

  Serial.print("Animation mode set to: ");
  Serial.println(getModeName(m_currentMode));

  // Initialize new mode
  switch (m_currentMode)
  {
  case MODE_OFF:
    if (m_standaloneAnimations != nullptr)
    {
      m_standaloneAnimations->off();
    }
    break;

  case MODE_SOLID:
    if (m_standaloneAnimations != nullptr)
    {
      // Use stored solid color
      m_standaloneAnimations->solid(m_solidColor);
    }
    break;

  case MODE_RAINBOW:
    // Will be updated in update() loop
    break;

  case MODE_BREATHING:
    // Will be updated in update() loop
    break;

  case MODE_CHASE:
    // Will be updated in update() loop
    break;

  case MODE_TWINKLE:
    // Will be updated in update() loop
    break;

  case MODE_FIRE:
    // Will be updated in update() loop
    break;

  case MODE_COLOR_WAVE:
    // Will be updated in update() loop
    break;

  case MODE_HEART_RATE_PULSE:
    if (m_heartRateAnimation != nullptr)
    {
      m_heartRateAnimation->reset();
    }
    break;

  default:
    break;
  }
}

bool AnimationManager::requiresHeartRate() const
{
  return modeRequiresHeartRate(m_currentMode);
}

void AnimationManager::update(uint16_t currentHeartRate)
{
  if (!m_initialized)
  {
    return;
  }

  switch (m_currentMode)
  {
  case MODE_OFF:
    // Nothing to update
    break;

  case MODE_SOLID:
    // Solid color doesn't need updates (set once)
    break;

  case MODE_RAINBOW:
    if (m_standaloneAnimations != nullptr)
    {
      m_standaloneAnimations->rainbow();
    }
    break;

  case MODE_BREATHING:
    if (m_standaloneAnimations != nullptr)
    {
      m_standaloneAnimations->breathing(m_solidColor);
    }
    break;

  case MODE_CHASE:
    if (m_standaloneAnimations != nullptr)
    {
      m_standaloneAnimations->chase(m_solidColor);
    }
    break;

  case MODE_TWINKLE:
    if (m_standaloneAnimations != nullptr)
    {
      m_standaloneAnimations->twinkle(m_solidColor);
    }
    break;

  case MODE_FIRE:
    if (m_standaloneAnimations != nullptr)
    {
      m_standaloneAnimations->fire();
    }
    break;

  case MODE_COLOR_WAVE:
    if (m_standaloneAnimations != nullptr)
    {
      m_standaloneAnimations->colorWave();
    }
    break;

  case MODE_HEART_RATE_PULSE:
    if (m_heartRateAnimation != nullptr)
    {
      CRGB pulseColor = m_solidColor;
      if (m_heartRateRainbowCycle && !m_heartRateRainbowInPulse)
      {
        uint8_t hue = (uint8_t)((millis() / HR_RAINBOW_MS_PER_HUE_STEP) & 0xFF);
        pulseColor = CHSV(hue, 255, 255);
      }
      m_heartRateAnimation->update(currentHeartRate, pulseColor,
                                   m_heartRateRainbowInPulse);
    }
    break;

  default:
    break;
  }
}

void AnimationManager::setSolidColor(uint8_t r, uint8_t g, uint8_t b)
{
  m_solidColor = CRGB(r, g, b);

  // If currently in a mode that uses color, update immediately
  if (m_standaloneAnimations != nullptr)
  {
    switch (m_currentMode)
    {
    case MODE_SOLID:
      m_standaloneAnimations->solid(m_solidColor);
      break;
    case MODE_BREATHING:
      m_standaloneAnimations->breathing(m_solidColor);
      break;
    case MODE_CHASE:
      m_standaloneAnimations->chase(m_solidColor);
      break;
    case MODE_TWINKLE:
      m_standaloneAnimations->twinkle(m_solidColor);
      break;
    default:
      break;
    }
  }

  Serial.print("Color set to RGB(");
  Serial.print(r);
  Serial.print(", ");
  Serial.print(g);
  Serial.print(", ");
  Serial.print(b);
  Serial.println(")");
}

void AnimationManager::setHeartRateRainbowCycle(bool enabled)
{
  m_heartRateRainbowCycle = enabled;
  Serial.print("Heart rate rainbow (global hue) ");
  Serial.println(enabled ? "on" : "off");
}

void AnimationManager::setHeartRateRainbowInPulse(bool enabled)
{
  m_heartRateRainbowInPulse = enabled;
  Serial.print("Heart rate rainbow (within pulse) ");
  Serial.println(enabled ? "on" : "off");
}

void AnimationManager::setSolidColor(uint32_t rgb)
{
  // Extract RGB from packed format (0xRRGGBB)
  uint8_t r = (rgb >> 16) & 0xFF;
  uint8_t g = (rgb >> 8) & 0xFF;
  uint8_t b = rgb & 0xFF;
  setSolidColor(r, g, b);
}

void AnimationManager::reset()
{
  if (m_heartRateAnimation != nullptr)
  {
    m_heartRateAnimation->reset();
  }
}
