#include <Arduino.h>
#include "BLEHeartRateMonitor.h"
#include "SerialUI.h"

// Global instances
BLEHeartRateMonitor heartRateMonitor;
SerialUI serialUI;

void setup()
{
  serialUI.begin(115200);

  // Initialize the heart rate monitor
  heartRateMonitor.begin();

  // Perform initial scan
  heartRateMonitor.scanForDevices();
}

void loop()
{
  // Update the monitor (handles connection state changes)
  heartRateMonitor.update();

  // Handle user input and device connection when not connected
  if (!heartRateMonitor.isConnected())
  {
    // Check if we should rescan
    if (heartRateMonitor.shouldRescan())
    {
      int userInput = serialUI.checkUserInput();

      if (userInput == -1)
      {
        // No input, perform automatic rescan
        heartRateMonitor.scanForDevices();
      }
      else if (userInput == 0)
      {
        // User requested rescan
        heartRateMonitor.scanForDevices();
      }
      else if (userInput > 0)
      {
        // User selected a device
        if (userInput <= (int)heartRateMonitor.getDiscoveredDeviceCount())
        {
          heartRateMonitor.connectToDevice(userInput);
        }
        else
        {
          serialUI.printError("Invalid device number");
        }
      }
    }
    else
    {
      // Check for user input while waiting for scan interval
      int userInput = serialUI.checkUserInput();
      if (userInput == 0)
      {
        heartRateMonitor.scanForDevices();
      }
      else if (userInput > 0 && userInput <= (int)heartRateMonitor.getDiscoveredDeviceCount())
      {
        heartRateMonitor.connectToDevice(userInput);
      }
    }
  }

  // Check for new heart rate data (already printed by callback, but could add custom handling here)
  if (heartRateMonitor.hasNewHeartRate())
  {
    // Heart rate is already printed by the notification callback
    // Add any additional processing here if needed
    // For example: uint16_t hr = heartRateMonitor.getCurrentHeartRate();
    heartRateMonitor.clearNewHeartRateFlag();
  }

  delay(100); // Small delay to prevent tight loop
}
