#ifndef ANIMATION_MODE_H
#define ANIMATION_MODE_H

enum AnimationMode
{
  MODE_OFF = 0,
  MODE_SOLID,
  MODE_RAINBOW,
  MODE_BREATHING,
  MODE_CHASE,
  MODE_TWINKLE,
  MODE_FIRE,
  MODE_COLOR_WAVE,
  MODE_HEART_RATE_PULSE,
  MODE_TEMPO_PULSE, // Same pulse visuals as heart rate; BPM from UI / BLE, not HR sensor
  MODE_COUNT        // Total number of modes
};

// Check if a mode requires BLE/heart rate data
inline bool modeRequiresHeartRate(AnimationMode mode)
{
  return mode == MODE_HEART_RATE_PULSE;
}

// Get mode name as string
const char *getModeName(AnimationMode mode);

#endif // ANIMATION_MODE_H
