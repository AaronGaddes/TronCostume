#include "HeartRateAnimation.h"
#include <math.h>

#ifndef PI
#define PI 3.14159265359f
#endif

HeartRateAnimation::HeartRateAnimation(LEDController *ledController)
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

void HeartRateAnimation::update(uint16_t currentHeartRate, CRGB pulseColor)
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
    // No valid heart rate data yet, show dim version of selected color
    CRGB dim = pulseColor;
    dim.nscale8(30);
    m_ledController->fill(dim);
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

  CRGB baseColor = pulseColor;

  // Get LED count for wave calculation
  uint16_t ledCount = m_ledController->getLEDCount();
  CRGB *leds = m_ledController->getLEDs();

  // Create traveling wave effect down the strip
  // Wave travels from start to end, synchronized with heart rate
  for (uint16_t i = 0; i < ledCount; i++)
  {
    // Calculate position along the strip (0.0 = start, 1.0 = end)
    float position = (float)i / (float)ledCount;

    // Calculate where the wave pulse is relative to this LED
    // m_pulsePhase goes from 0.0 to 1.0 as the wave travels down
    // Position of wave front: m_pulsePhase * ledCount (in LED units)
    // Distance from LED to wave front: (m_pulsePhase * ledCount) - i
    float wavePosition = m_pulsePhase * ledCount;
    float distanceFromWave = wavePosition - (float)i;

    // Normalize distance to create a pulse width
    // Negative = wave hasn't reached this LED yet
    // 0 = wave is at this LED (brightest)
    // Positive = wave has passed this LED
    float pulseWidth = 8.0f; // Width of the pulse in LEDs
    float normalizedDistance = distanceFromWave / pulseWidth;

    // Calculate brightness based on distance from wave front
    uint8_t brightness;
    if (normalizedDistance < -1.0f || normalizedDistance > 1.0f)
    {
      // Too far from wave, use base brightness
      brightness = PULSE_BASE_BRIGHTNESS;
    }
    else
    {
      // Within pulse range, calculate brightness using bell curve
      float bellCurve = exp(-(normalizedDistance * normalizedDistance) / 2.0f);
      brightness = PULSE_BASE_BRIGHTNESS +
                   (uint8_t)(bellCurve * (PULSE_MAX_BRIGHTNESS - PULSE_BASE_BRIGHTNESS));
    }

    // Apply color with brightness
    CRGB ledColor = baseColor;
    ledColor.nscale8(brightness);
    leds[i] = ledColor;
  }

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

uint8_t HeartRateAnimation::calculateWaveBrightness(float phase) const
{
  // Create a traveling wave pulse effect
  // Use a bell curve shape for smooth wave that travels down the strip
  // Phase represents position in the wave (0.0 = start, 1.0 = end)

  // Create a pulse that's brightest in the middle of the wave
  // Use a Gaussian-like curve for smooth falloff
  float center = 0.5f; // Wave is brightest at center
  float width = 0.3f;  // Width of the pulse wave

  // Calculate distance from center
  float distance = fabs(phase - center);

  // Create bell curve: exp(-(distance^2) / (2 * width^2))
  float normalizedDistance = distance / width;
  float bellCurve = exp(-(normalizedDistance * normalizedDistance) / 2.0f);

  // Scale to brightness range
  uint8_t brightness = PULSE_BASE_BRIGHTNESS +
                       (uint8_t)(bellCurve * (PULSE_MAX_BRIGHTNESS - PULSE_BASE_BRIGHTNESS));

  return brightness;
}
