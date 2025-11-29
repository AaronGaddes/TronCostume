#include "StandaloneAnimations.h"
#include <math.h>

#ifndef PI
#define PI 3.14159265359f
#endif

StandaloneAnimations::StandaloneAnimations(LEDController *ledController)
    : m_ledController(ledController),
      m_lastUpdateTime(0),
      m_rainbowHue(0),
      m_breathingPhase(0.0f),
      m_chasePosition(0),
      m_twinkleLastUpdate(0),
      m_colorWavePhase(0.0f)
{
  // Initialize twinkle sparkles
  for (int i = 0; i < 30; i++)
  {
    m_twinkleSparkles[i] = 0;
  }
}

StandaloneAnimations::~StandaloneAnimations()
{
}

void StandaloneAnimations::off()
{
  if (m_ledController == nullptr)
  {
    return;
  }

  m_ledController->clear();
  m_ledController->show();
}

void StandaloneAnimations::solid(CRGB color)
{
  if (m_ledController == nullptr)
  {
    return;
  }

  m_ledController->fill(color);
  m_ledController->show();
}

void StandaloneAnimations::rainbow()
{
  if (m_ledController == nullptr)
  {
    return;
  }

  unsigned long currentTime = millis();
  unsigned long deltaTime = currentTime - m_lastUpdateTime;

  // Update rainbow every 20ms for smooth animation
  if (deltaTime >= 20)
  {
    // Fill LEDs with rainbow gradient
    fill_rainbow(m_ledController->getLEDs(),
                 m_ledController->getLEDCount(),
                 m_rainbowHue,
                 7); // 7 = hue increment per LED for nice gradient

    m_ledController->show();

    // Increment hue for next frame
    m_rainbowHue += 2;
    if (m_rainbowHue >= 256)
    {
      m_rainbowHue = 0;
    }

    m_lastUpdateTime = currentTime;
  }
}

void StandaloneAnimations::breathing()
{
  if (m_ledController == nullptr)
  {
    return;
  }

  unsigned long currentTime = millis();
  unsigned long deltaTime = currentTime - m_lastUpdateTime;

  // Update breathing every 20ms
  if (deltaTime >= 20)
  {
    uint16_t ledCount = m_ledController->getLEDCount();
    CRGB *leds = m_ledController->getLEDs();

    // Update breathing phase (sine wave, 0.0 to 1.0)
    float breathingSpeed = 0.002f; // Speed of breathing
    m_breathingPhase += breathingSpeed * deltaTime;
    if (m_breathingPhase >= 1.0f)
    {
      m_breathingPhase -= 1.0f;
    }

    // Calculate brightness using sine wave (smooth pulse)
    float sineValue = sin(m_breathingPhase * 2.0f * PI);
    float brightness = (sineValue + 1.0f) * 0.5f;               // 0.0 to 1.0
    uint8_t brightnessValue = 50 + (uint8_t)(brightness * 205); // 50 to 255

    // Apply cyan color with breathing brightness
    CRGB color = CRGB(0, brightnessValue, brightnessValue);
    m_ledController->fill(color);
    m_ledController->show();

    m_lastUpdateTime = currentTime;
  }
}

void StandaloneAnimations::chase()
{
  if (m_ledController == nullptr)
  {
    return;
  }

  unsigned long currentTime = millis();
  unsigned long deltaTime = currentTime - m_lastUpdateTime;

  // Update chase every 30ms
  if (deltaTime >= 30)
  {
    uint16_t ledCount = m_ledController->getLEDCount();
    CRGB *leds = m_ledController->getLEDs();

    // Clear all LEDs
    m_ledController->clear();

    // Move chase position
    m_chasePosition = (m_chasePosition + 1) % (ledCount * 2);

    // Calculate actual position (back and forth)
    uint16_t pos;
    if (m_chasePosition < ledCount)
    {
      pos = m_chasePosition;
    }
    else
    {
      pos = (ledCount * 2) - m_chasePosition - 1;
    }

    // Set chase light (white with trail)
    for (int i = -2; i <= 2; i++)
    {
      int ledIndex = pos + i;
      if (ledIndex >= 0 && ledIndex < (int)ledCount)
      {
        uint8_t brightness = 255;
        if (abs(i) == 1)
          brightness = 128;
        if (abs(i) == 2)
          brightness = 64;
        leds[ledIndex] = CRGB(brightness, brightness, brightness);
      }
    }

    m_ledController->show();
    m_lastUpdateTime = currentTime;
  }
}

void StandaloneAnimations::twinkle()
{
  if (m_ledController == nullptr)
  {
    return;
  }

  unsigned long currentTime = millis();
  unsigned long deltaTime = currentTime - m_twinkleLastUpdate;

  // Update twinkle every 50ms
  if (deltaTime >= 50)
  {
    uint16_t ledCount = m_ledController->getLEDCount();
    CRGB *leds = m_ledController->getLEDs();

    // Fade existing sparkles
    for (uint16_t i = 0; i < ledCount && i < 30; i++)
    {
      if (m_twinkleSparkles[i] > 0)
      {
        int newValue = (int)m_twinkleSparkles[i] - 10;
        m_twinkleSparkles[i] = (newValue > 0) ? (uint8_t)newValue : 0;
      }
    }

    // Randomly add new sparkles
    if (random(100) < 15) // 15% chance per update
    {
      uint16_t sparkleIndex = random(ledCount);
      if (sparkleIndex < 30)
      {
        m_twinkleSparkles[sparkleIndex] = 255;
      }
    }

    // Apply sparkles to LEDs
    for (uint16_t i = 0; i < ledCount && i < 30; i++)
    {
      if (m_twinkleSparkles[i] > 0)
      {
        // Random color for each sparkle
        uint8_t hue = random(256);
        leds[i] = CHSV(hue, 255, m_twinkleSparkles[i]);
      }
      else
      {
        leds[i] = CRGB::Black;
      }
    }

    m_ledController->show();
    m_twinkleLastUpdate = currentTime;
  }
}

void StandaloneAnimations::fire()
{
  if (m_ledController == nullptr)
  {
    return;
  }

  unsigned long currentTime = millis();
  unsigned long deltaTime = currentTime - m_lastUpdateTime;

  // Update fire every 30ms
  if (deltaTime >= 30)
  {
    uint16_t ledCount = m_ledController->getLEDCount();
    CRGB *leds = m_ledController->getLEDs();

    // Fire effect: base (red/orange) with yellow flicker
    for (uint16_t i = 0; i < ledCount; i++)
    {
      // Base fire color (red to orange gradient)
      uint8_t baseRed = 255;
      uint8_t baseGreen = (i * 100) / ledCount; // Gradient from red to orange
      uint8_t baseBlue = 0;

      // Add random flicker
      uint8_t flicker = random(0, 50);
      uint8_t red = min(255, baseRed - flicker);
      uint8_t green = min(255, baseGreen + flicker / 2);

      // Add yellow highlights randomly
      if (random(100) < 10)
      {
        green = min(255, green + 100);
      }

      leds[i] = CRGB(red, green, baseBlue);
    }

    m_ledController->show();
    m_lastUpdateTime = currentTime;
  }
}

void StandaloneAnimations::colorWave()
{
  if (m_ledController == nullptr)
  {
    return;
  }

  unsigned long currentTime = millis();
  unsigned long deltaTime = currentTime - m_lastUpdateTime;

  // Update color wave every 20ms
  if (deltaTime >= 20)
  {
    uint16_t ledCount = m_ledController->getLEDCount();
    CRGB *leds = m_ledController->getLEDs();

    // Update wave phase
    float waveSpeed = 0.005f;
    m_colorWavePhase += waveSpeed * deltaTime;
    if (m_colorWavePhase >= 1.0f)
    {
      m_colorWavePhase -= 1.0f;
    }

    // Create color wave traveling down the strip
    for (uint16_t i = 0; i < ledCount; i++)
    {
      // Calculate position in wave (0.0 to 1.0)
      float position = (float)i / (float)ledCount;

      // Calculate hue based on position and phase
      float hue = (position + m_colorWavePhase) * 255.0f;
      if (hue >= 256.0f)
        hue -= 256.0f;

      // Full saturation and brightness
      leds[i] = CHSV((uint8_t)hue, 255, 255);
    }

    m_ledController->show();
    m_lastUpdateTime = currentTime;
  }
}

void StandaloneAnimations::update()
{
  // This can be called to update time-based animations
  // Currently only rainbow needs updates
  // Other methods handle their own updates
}
