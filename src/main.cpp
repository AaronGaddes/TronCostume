#include <Arduino.h>
#include "BLEHeartRateMonitor.h"
#include "BLEControlService.h"
#include "SerialUI.h"
#include "LEDController.h"
#include "AnimationManager.h"
#include "AnimationMode.h"

// Global instances
BLEHeartRateMonitor heartRateMonitor;
SerialUI serialUI;
LEDController ledController;
AnimationManager animationManager(&ledController);
BLEControlService bleControlService(&animationManager);

// State tracking
bool modeMenuShown = false;

void setup()
{
  serialUI.begin(115200);
  delay(1000); // Give serial time to initialize

  Serial.println("\n=== Tron Costume LED Controller ===");
  Serial.println("Initializing...");

  // Initialize LED controller
  if (!ledController.begin())
  {
    Serial.println("ERROR: Failed to initialize LED controller");
    while (1)
      delay(1000); // Halt on error
  }

  // Initialize animation manager
  if (!animationManager.begin())
  {
    Serial.println("ERROR: Failed to initialize animation manager");
    while (1)
      delay(1000); // Halt on error
  }

  // Initialize BLE Control Service (for frontend connection)
  if (!bleControlService.begin())
  {
    Serial.println("ERROR: Failed to initialize BLE Control Service");
    while (1)
      delay(1000); // Halt on error
  }

  // Start with RAINBOW mode
  animationManager.setMode(MODE_RAINBOW);
  bleControlService.setCurrentMode(MODE_RAINBOW);
  serialUI.displayCurrentMode(animationManager.getCurrentMode());

  // Display mode menu
  serialUI.displayModeMenu();
  modeMenuShown = true;

  Serial.println("\nSystem ready!");
}

void loop()
{
  // Update BLE Control Service (handles connections and notifications)
  bleControlService.update();

  // Check for mode selection from BLE
  if (bleControlService.hasModeChanged())
  {
    AnimationMode newMode = bleControlService.getCurrentMode();
    AnimationMode currentMode = animationManager.getCurrentMode();

    // Check if we need to initialize/shutdown BLE for heart rate
    bool newModeNeedsHR = modeRequiresHeartRate(newMode);
    bool currentModeNeedsHR = modeRequiresHeartRate(currentMode);

    // If switching to a mode that needs HR, initialize BLE
    if (newModeNeedsHR && !heartRateMonitor.isInitialized())
    {
      Serial.println("Initializing BLE for heart rate mode...");
      if (!heartRateMonitor.begin())
      {
        Serial.println("ERROR: Failed to initialize BLE");
        bleControlService.clearModeChangedFlag();
        return;
      }
    }

    // If switching away from HR mode, disconnect the heart rate monitor client
    // (but keep BLE stack running for the control service)
    if (!newModeNeedsHR && currentModeNeedsHR && heartRateMonitor.isConnected())
    {
      Serial.println("Disconnecting heart rate monitor (not needed for current mode)...");
      heartRateMonitor.disconnect();
    }

    // Set the new mode
    animationManager.setMode(newMode);
    // Sync the mode back to BLE service to ensure it's in sync and notify clients
    bleControlService.setCurrentMode(newMode);
    // Force notification to ensure frontend stays in sync
    bleControlService.notifyCurrentMode();
    serialUI.displayCurrentMode(newMode);

    // If switching to heart rate mode and BLE is initialized but not connected, auto-connect to first heart rate device
    if (newModeNeedsHR && heartRateMonitor.isInitialized() && !heartRateMonitor.isConnected())
    {
      Serial.println("Attempting to auto-connect to heart rate device...");
      if (!heartRateMonitor.connectToFirstHeartRateDevice())
      {
        Serial.println("Failed to auto-connect to heart rate device. Will retry on next scan interval.");
      }
    }

    bleControlService.clearModeChangedFlag();
  }

  // Check for mode selection input from serial
  int modeSelection = serialUI.checkModeSelection();
  if (modeSelection >= 0)
  {
    AnimationMode newMode = (AnimationMode)modeSelection;
    AnimationMode currentMode = animationManager.getCurrentMode();

    // Check if we need to initialize/shutdown BLE
    bool newModeNeedsHR = modeRequiresHeartRate(newMode);
    bool currentModeNeedsHR = modeRequiresHeartRate(currentMode);

    // If switching to a mode that needs HR, initialize BLE
    if (newModeNeedsHR && !heartRateMonitor.isInitialized())
    {
      Serial.println("Initializing BLE for heart rate mode...");
      if (!heartRateMonitor.begin())
      {
        Serial.println("ERROR: Failed to initialize BLE");
        return;
      }
    }

    // If switching away from HR mode, disconnect the heart rate monitor client
    // (but keep BLE stack running for the control service)
    if (!newModeNeedsHR && currentModeNeedsHR && heartRateMonitor.isConnected())
    {
      Serial.println("Disconnecting heart rate monitor (not needed for current mode)...");
      heartRateMonitor.disconnect();
    }

    // Set the new mode
    animationManager.setMode(newMode);
    bleControlService.setCurrentMode(newMode);
    serialUI.displayCurrentMode(newMode);

    // If switching to heart rate mode and BLE is initialized but not connected, auto-connect to first heart rate device
    if (newModeNeedsHR && heartRateMonitor.isInitialized() && !heartRateMonitor.isConnected())
    {
      Serial.println("Attempting to auto-connect to heart rate device...");
      if (!heartRateMonitor.connectToFirstHeartRateDevice())
      {
        Serial.println("Failed to auto-connect to heart rate device. Will retry on next scan interval.");
      }
    }
  }

  // Handle BLE operations only if heart rate mode is active
  if (animationManager.requiresHeartRate() && heartRateMonitor.isInitialized())
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
          // No input, perform automatic connection to first heart rate device
          if (!heartRateMonitor.connectToFirstHeartRateDevice())
          {
            Serial.println("Auto-connect failed. Use serial input to manually select a device.");
          }
        }
        else if (userInput == 0)
        {
          // User requested rescan
          heartRateMonitor.scanForDevices();
        }
        else if (userInput > 0)
        {
          // User selected a device manually
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

    // Get current heart rate for animation
    uint16_t currentHeartRate = 0;
    if (heartRateMonitor.hasNewHeartRate())
    {
      currentHeartRate = heartRateMonitor.getCurrentHeartRate();
      heartRateMonitor.clearNewHeartRateFlag();
    }
    else if (heartRateMonitor.isConnected())
    {
      // Use last known heart rate even if no new update
      currentHeartRate = heartRateMonitor.getCurrentHeartRate();
    }

    // Update animation with heart rate
    animationManager.update(currentHeartRate);
  }
  else
  {
    // Update animation without heart rate (standalone modes)
    animationManager.update(0);
  }

  delay(10); // Small delay to prevent tight loop
}
