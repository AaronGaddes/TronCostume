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

void HeartRateAnimation::update(uint16_t rateBpm, CRGB pulseColor,
                                bool rainbowInPulse, bool fixedTempoMode,
                                uint8_t beatsPerMeasure)
{
  if (m_ledController == nullptr)
  {
    return;
  }

  unsigned long currentTime = millis();

  uint16_t avgHeartRate = 0;

  if (fixedTempoMode)
  {
    if (rateBpm > 0)
    {
      avgHeartRate = constrain(rateBpm, MIN_TEMPO_BPM, MAX_TEMPO_BPM);
    }
  }
  else
  {
    if (rateBpm > 0)
    {
      addHeartRateSample(rateBpm);
    }
    avgHeartRate = getAverageHeartRate();
  }

  if (avgHeartRate == 0)
  {
    // No valid rate yet (HR) or zero BPM (tempo), show dim version of selected color
    CRGB dim = pulseColor;
    dim.nscale8(30);
    m_ledController->fill(dim);
    m_ledController->show();
    return;
  }

  // Heart rate: slight speed variation by BPM; fixed tempo: strict BPM timing
  float pulseSpeed =
      fixedTempoMode ? 1.0f : calculatePulseSpeed(avgHeartRate);

  // Update pulse phase (0.0 to 1.0)
  unsigned long deltaTime = currentTime - m_lastUpdateTime;
  if (m_lastUpdateTime > 0 && deltaTime < 1000) // Prevent overflow
  {
    float msPerBeat = 60000.0f / (float)avgHeartRate;
    float msForOneWaveCycle = msPerBeat;
    if (fixedTempoMode && beatsPerMeasure > 1)
    {
      // One full strip traversal per measure (beatsPerMeasure quarter-note beats)
      msForOneWaveCycle = msPerBeat * (float)beatsPerMeasure;
    }
    float phaseIncrement = (deltaTime / msForOneWaveCycle) * pulseSpeed;
    m_pulsePhase += phaseIncrement;

    // Wrap phase to 0-1 range
    while (m_pulsePhase >= 1.0f)
    {
      m_pulsePhase -= 1.0f;
    }
  }
  m_lastUpdateTime = currentTime;

  const bool measureSubdivision =
      fixedTempoMode && beatsPerMeasure > 1;

  float beatBumpGlobal = 0.0f;
  if (measureSubdivision)
  {
    float beatPhase = m_pulsePhase * (float)beatsPerMeasure;
    float frac = beatPhase - floorf(beatPhase);
    float distBeat = fminf(frac, 1.0f - frac);
    float z = distBeat / TEMPO_BEAT_SIGMA;
    beatBumpGlobal = (float)TEMPO_BEAT_BUMP_MAX * expf(-z * z);
  }

  // Get LED count for wave calculation
  uint16_t ledCount = m_ledController->getLEDCount();
  CRGB *leds = m_ledController->getLEDs();

  // Create traveling wave effect down the strip
  // Wave travels from start to end, synchronized with heart rate
  for (uint16_t i = 0; i < ledCount; i++)
  {
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

    float localBell = 0.0f;
    if (normalizedDistance >= -1.0f && normalizedDistance <= 1.0f)
    {
      localBell =
          expf(-(normalizedDistance * normalizedDistance) / 2.0f);
    }

    // Calculate brightness based on distance from wave front
    uint8_t brightness;
    if (normalizedDistance < -1.0f || normalizedDistance > 1.0f)
    {
      // Too far from wave, use base brightness
      brightness = PULSE_BASE_BRIGHTNESS;
    }
    else
    {
      float mainGain = measureSubdivision ? TEMPO_MAIN_BELL_GAIN : 1.0f;
      float span = (float)(PULSE_MAX_BRIGHTNESS - PULSE_BASE_BRIGHTNESS);
      int waveBright = (int)PULSE_BASE_BRIGHTNESS +
                       (int)(localBell * span * mainGain);
      if (waveBright > 255)
      {
        waveBright = 255;
      }
      brightness = (uint8_t)waveBright;
    }

    if (measureSubdivision)
    {
      // Stronger beat flash in dim regions; weaker under main wave crest
      float beatScale = 1.0f - 0.72f * localBell;
      if (beatScale < 0.12f)
      {
        beatScale = 0.12f;
      }
      int b = (int)brightness + (int)(beatBumpGlobal * beatScale);
      if (b > 255)
      {
        b = 255;
      }
      brightness = (uint8_t)b;
    }

    CRGB ledColor;
    if (rainbowInPulse &&
        normalizedDistance >= -1.0f && normalizedDistance <= 1.0f)
    {
      // Full spectrum across the pulse width, shifted as the wave travels (m_pulsePhase 0..1)
      float t = (normalizedDistance + 1.0f) * 0.5f;
      uint8_t hueAlong = (uint8_t)(t * 255.0f);
      uint8_t hueScroll = (uint8_t)(m_pulsePhase * 255.0f);
      ledColor = CHSV((uint8_t)(hueAlong + hueScroll), 255, 255);
      ledColor.nscale8(brightness);
    }
    else
    {
      ledColor = pulseColor;
      ledColor.nscale8(brightness);
    }
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
