#include "AnimationMode.h"

const char *getModeName(AnimationMode mode)
{
  switch (mode)
  {
  case MODE_OFF:
    return "Off";
  case MODE_SOLID:
    return "Solid";
  case MODE_RAINBOW:
    return "Rainbow";
  case MODE_BREATHING:
    return "Breathing";
  case MODE_CHASE:
    return "Chase";
  case MODE_TWINKLE:
    return "Twinkle";
  case MODE_FIRE:
    return "Fire";
  case MODE_COLOR_WAVE:
    return "Color Wave";
  case MODE_HEART_RATE_PULSE:
    return "Heart Rate Pulse";
  default:
    return "Unknown";
  }
}
