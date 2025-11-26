#include "StandaloneAnimations.h"

StandaloneAnimations::StandaloneAnimations(LEDController* ledController)
    : m_ledController(ledController),
      m_lastUpdateTime(0),
      m_rainbowHue(0)
{
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

void StandaloneAnimations::update()
{
  // This can be called to update time-based animations
  // Currently only rainbow needs updates
  // Other methods handle their own updates
}

