#ifndef HEART_RATE_ANIMATION_H
#define HEART_RATE_ANIMATION_H

#include "LEDController.h"
#include <Arduino.h>

// Configuration
#define ROLLING_AVERAGE_SIZE 15
#define MIN_HEART_RATE 60
#define MAX_HEART_RATE 180
#define MIN_TEMPO_BPM 40
#define MAX_TEMPO_BPM 240
#define PULSE_BASE_BRIGHTNESS 50
#define PULSE_MAX_BRIGHTNESS 255
// Tempo + time signature: subtle per-beat flash, stronger main wave once per measure
#define TEMPO_BEAT_BUMP_MAX 30
#define TEMPO_BEAT_SIGMA 0.09f
#define TEMPO_MAIN_BELL_GAIN 1.18f

class HeartRateAnimation
{
public:
  HeartRateAnimation(LEDController *ledController);
  ~HeartRateAnimation();

  // pulseColor: base/dim color. rainbowInPulse: spread+scroll rainbow across the bright wave.
  // fixedTempoMode: use rateBpm directly (no rolling average).
  // beatsPerMeasure: 1 = one beat per wave (heart rate); 2–4 = one wave per bar + subtle beat taps.
  void update(uint16_t rateBpm, CRGB pulseColor, bool rainbowInPulse,
              bool fixedTempoMode = false, uint8_t beatsPerMeasure = 1);

  // Reset animation state
  void reset();

private:
  LEDController *m_ledController;

  // Rolling average buffer
  uint16_t m_heartRateBuffer[ROLLING_AVERAGE_SIZE];
  uint8_t m_bufferIndex;
  uint8_t m_bufferCount;

  // Animation state
  unsigned long m_lastUpdateTime;
  float m_pulsePhase; // 0.0 to 1.0 for pulse cycle

  // Helper methods
  void addHeartRateSample(uint16_t heartRate);
  uint16_t getAverageHeartRate() const;
  float calculatePulseSpeed(uint16_t avgHeartRate) const;
  uint8_t calculateBrightness(float phase, float speed) const;
  uint8_t calculateWaveBrightness(float phase) const;
};

#endif // HEART_RATE_ANIMATION_H
