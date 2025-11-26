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

