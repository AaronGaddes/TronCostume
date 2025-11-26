#include "LEDController.h"
#include <Arduino.h>

LEDController::LEDController()
    : m_leds(nullptr), m_ledCount(0), m_initialized(false)
{
}

LEDController::~LEDController()
{
  if (m_leds != nullptr)
  {
    delete[] m_leds;
  }
}

bool LEDController::begin(uint16_t ledCount)
{
  if (m_initialized)
  {
    return true; // Already initialized
  }

  m_ledCount = ledCount;
  m_leds = new CRGB[m_ledCount];

  if (m_leds == nullptr)
  {
    Serial.println("ERROR: Failed to allocate memory for LEDs");
    return false;
  }

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(m_leds, m_ledCount);
  FastLED.setMaxPowerInVoltsAndMilliamps(MAX_VOLTAGE, MAX_MILLI_AMPS);
  FastLED.setBrightness(128); // Default brightness
  FastLED.clear();
  FastLED.show();

  m_initialized = true;
  Serial.print("LED Controller initialized with ");
  Serial.print(m_ledCount);
  Serial.println(" LEDs");

  return true;
}

void LEDController::clear()
{
  if (m_initialized)
  {
    FastLED.clear();
  }
}

void LEDController::show()
{
  if (m_initialized)
  {
    FastLED.show();
  }
}

void LEDController::setPixel(uint16_t index, CRGB color)
{
  if (m_initialized && index < m_ledCount)
  {
    m_leds[index] = color;
  }
}

void LEDController::fill(CRGB color)
{
  if (m_initialized)
  {
    fill_solid(m_leds, m_ledCount, color);
  }
}

void LEDController::fill(CRGB color, uint16_t first, uint16_t count)
{
  if (m_initialized && first < m_ledCount)
  {
    uint16_t end = (first + count < m_ledCount) ? (first + count) : m_ledCount;
    for (uint16_t i = first; i < end; i++)
    {
      m_leds[i] = color;
    }
  }
}

void LEDController::setBrightness(uint8_t brightness)
{
  if (m_initialized)
  {
    FastLED.setBrightness(brightness);
  }
}

uint8_t LEDController::getBrightness() const
{
  if (m_initialized)
  {
    return FastLED.getBrightness();
  }
  return 0;
}
