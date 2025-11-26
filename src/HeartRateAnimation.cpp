#include "HeartRateAnimation.h"
#include <math.h>

#ifndef PI
#define PI 3.14159265359f
#endif

HeartRateAnimation::HeartRateAnimation(LEDController* ledController)
    : m_ledController(ledController),
      m_bufferIndex(0),
      m_bufferCount(0),
      m_lastUpdateTime(0),
      m_pulsePhase(0.0f)
{
  // Initialize buffer
  for (int i = 0; i < ROLLING_AVERAGE_SIZE; i++)
  {
    m_heartRateBuffer[i] = 0;
  }
}

HeartRateAnimation::~HeartRateAnimation()
{
}

void HeartRateAnimation::update(uint16_t currentHeartRate)
{
  if (m_ledController == nullptr)
  {
    return;
  }

  unsigned long currentTime = millis();
  
  // Add new heart rate sample to rolling average
  if (currentHeartRate > 0)
  {
    addHeartRateSample(currentHeartRate);
  }

  // Get averaged heart rate
  uint16_t avgHeartRate = getAverageHeartRate();
  
  if (avgHeartRate == 0)
  {
    // No valid heart rate data yet, show dim red
    m_ledController->fill(CRGB(20, 0, 0));
    m_ledController->show();
    return;
  }

  // Calculate pulse speed based on heart rate
  float pulseSpeed = calculatePulseSpeed(avgHeartRate);
  
  // Update pulse phase (0.0 to 1.0)
  unsigned long deltaTime = currentTime - m_lastUpdateTime;
  if (m_lastUpdateTime > 0 && deltaTime < 1000) // Prevent overflow
  {
    // Convert heart rate to milliseconds per beat
    float msPerBeat = 60000.0f / avgHeartRate;
    float phaseIncrement = (deltaTime / msPerBeat) * pulseSpeed;
    m_pulsePhase += phaseIncrement;
    
    // Wrap phase to 0-1 range
    while (m_pulsePhase >= 1.0f)
    {
      m_pulsePhase -= 1.0f;
    }
  }
  m_lastUpdateTime = currentTime;

  // Calculate brightness based on pulse phase (sine wave for smooth pulse)
  uint8_t brightness = calculateBrightness(m_pulsePhase, pulseSpeed);
  
  // Get heart rate color (red)
  CRGB color = getHeartRateColor();
  
  // Scale color by brightness
  color.nscale8(brightness);
  
  // Apply to all LEDs
  m_ledController->fill(color);
  m_ledController->show();
}

void HeartRateAnimation::reset()
{
  m_bufferIndex = 0;
  m_bufferCount = 0;
  m_pulsePhase = 0.0f;
  m_lastUpdateTime = 0;
  
  for (int i = 0; i < ROLLING_AVERAGE_SIZE; i++)
  {
    m_heartRateBuffer[i] = 0;
  }
}

void HeartRateAnimation::addHeartRateSample(uint16_t heartRate)
{
  // Clamp heart rate to reasonable range
  if (heartRate < MIN_HEART_RATE)
  {
    heartRate = MIN_HEART_RATE;
  }
  else if (heartRate > MAX_HEART_RATE)
  {
    heartRate = MAX_HEART_RATE;
  }

  m_heartRateBuffer[m_bufferIndex] = heartRate;
  m_bufferIndex = (m_bufferIndex + 1) % ROLLING_AVERAGE_SIZE;
  
  if (m_bufferCount < ROLLING_AVERAGE_SIZE)
  {
    m_bufferCount++;
  }
}

uint16_t HeartRateAnimation::getAverageHeartRate() const
{
  if (m_bufferCount == 0)
  {
    return 0;
  }

  uint32_t sum = 0;
  for (uint8_t i = 0; i < m_bufferCount; i++)
  {
    sum += m_heartRateBuffer[i];
  }

  return sum / m_bufferCount;
}

float HeartRateAnimation::calculatePulseSpeed(uint16_t avgHeartRate) const
{
  // Map heart rate to pulse speed multiplier
  // Higher heart rate = faster pulse
  // Normalize to 0.8 - 1.2 range
  float normalized = (float)(avgHeartRate - MIN_HEART_RATE) / (MAX_HEART_RATE - MIN_HEART_RATE);
  normalized = constrain(normalized, 0.0f, 1.0f);
  return 0.8f + (normalized * 0.4f); // 0.8x to 1.2x speed
}

uint8_t HeartRateAnimation::calculateBrightness(float phase, float speed) const
{
  // Use sine wave for smooth pulsing
  // Phase 0.0 = dim, 0.5 = bright, 1.0 = dim
  float sineValue = sin(phase * 2.0f * PI);
  
  // Map from -1..1 to 0..1
  float normalized = (sineValue + 1.0f) * 0.5f;
  
  // Scale to brightness range
  uint8_t brightness = PULSE_BASE_BRIGHTNESS + 
                       (uint8_t)(normalized * (PULSE_MAX_BRIGHTNESS - PULSE_BASE_BRIGHTNESS));
  
  return brightness;
}

CRGB HeartRateAnimation::getHeartRateColor() const
{
  // Red color for heart rate
  return CRGB(255, 0, 0);
}

