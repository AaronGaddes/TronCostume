#include "SerialUI.h"

SerialUI::SerialUI()
{
}

void SerialUI::begin(unsigned long baudRate)
{
  Serial.begin(baudRate);
}

int SerialUI::checkUserInput()
{
  if (Serial.available() > 0)
  {
    String input = Serial.readStringUntil('\n');
    input.trim();
    return input.toInt();
  }
  return -1;
}

int SerialUI::checkModeSelection()
{
  if (Serial.available() > 0)
  {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toLowerCase();
    
    // Check for mode commands (m0, m1, m2, etc.)
    if (input.length() >= 2 && input[0] == 'm')
    {
      int modeNum = input.substring(1).toInt();
      if (modeNum >= 0 && modeNum < MODE_COUNT)
      {
        return modeNum;
      }
    }
    
    // Check for direct numeric input
    int modeNum = input.toInt();
    if (modeNum >= 0 && modeNum < MODE_COUNT)
    {
      return modeNum;
    }
  }
  return -1;
}

void SerialUI::displayModeMenu()
{
  Serial.println("\n=== Animation Mode Selection ===");
  for (int i = 0; i < MODE_COUNT; i++)
  {
    Serial.print(i);
    Serial.print(" - ");
    Serial.print(getModeName((AnimationMode)i));
    if (modeRequiresHeartRate((AnimationMode)i))
    {
      Serial.print(" (requires heart rate)");
    }
    Serial.println();
  }
  Serial.println("Enter mode number (0-");
  Serial.print(MODE_COUNT - 1);
  Serial.println(") or 'm' followed by number (e.g., m0, m1):");
}

void SerialUI::displayCurrentMode(AnimationMode mode)
{
  Serial.print("Current mode: ");
  Serial.print(getModeName(mode));
  if (modeRequiresHeartRate(mode))
  {
    Serial.print(" (requires heart rate)");
  }
  Serial.println();
}

void SerialUI::printPrompt(const char* message)
{
  Serial.println(message);
}

void SerialUI::printError(const char* message)
{
  Serial.print("ERROR: ");
  Serial.println(message);
}

void SerialUI::printInfo(const char* message)
{
  Serial.println(message);
}

void SerialUI::printHeartRate(uint16_t heartRate)
{
  Serial.print("Heart Rate: ");
  Serial.print(heartRate);
  Serial.println(" bpm");
}

