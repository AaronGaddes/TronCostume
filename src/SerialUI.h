#ifndef SERIAL_UI_H
#define SERIAL_UI_H

#include <Arduino.h>
#include "AnimationMode.h"

class SerialUI
{
public:
  SerialUI();
  
  // Input handling
  int checkUserInput();
  bool hasInput() const { return Serial.available() > 0; }
  
  // Mode selection
  int checkModeSelection(); // Returns mode number or -1 if no valid input
  void displayModeMenu();
  void displayCurrentMode(AnimationMode mode);
  
  // Output helpers
  void printPrompt(const char* message);
  void printError(const char* message);
  void printInfo(const char* message);
  void printHeartRate(uint16_t heartRate);
  
  // Initialization
  void begin(unsigned long baudRate = 115200);
};

#endif // SERIAL_UI_H

