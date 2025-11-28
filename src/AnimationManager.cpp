#include "AnimationManager.h"
#include <Arduino.h>

AnimationManager::AnimationManager(LEDController *ledController)
    : m_ledController(ledController),
      m_heartRateAnimation(nullptr),
      m_standaloneAnimations(nullptr),
      m_currentMode(MODE_OFF),
      m_initialized(false),
      m_solidColor(CRGB::White) // Default to white
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

  case MODE_HEART_RATE_PULSE:
    if (m_heartRateAnimation != nullptr)
    {
      m_heartRateAnimation->update(currentHeartRate);
    }
    break;

  default:
    break;
  }
}

void AnimationManager::setSolidColor(uint8_t r, uint8_t g, uint8_t b)
{
  m_solidColor = CRGB(r, g, b);

  // If currently in solid mode, update immediately
  if (m_currentMode == MODE_SOLID && m_standaloneAnimations != nullptr)
  {
    m_standaloneAnimations->solid(m_solidColor);
  }

  Serial.print("Solid color set to RGB(");
  Serial.print(r);
  Serial.print(", ");
  Serial.print(g);
  Serial.print(", ");
  Serial.print(b);
  Serial.println(")");
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
