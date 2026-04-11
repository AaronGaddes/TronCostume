#ifndef BLE_CONTROL_SERVICE_H
#define BLE_CONTROL_SERVICE_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "AnimationMode.h"

#define DEVICE_NAME "ESP32-LED-Controller"
// BLE Service UUIDs
#define CONTROL_SERVICE_UUID "19B10000-E8F2-537E-4F6C-D104768A1214"
#define MODE_CONTROL_CHAR_UUID "19B10001-E8F2-537E-4F6C-D104768A1214"
#define MODE_STATUS_CHAR_UUID "19B10002-E8F2-537E-4F6C-D104768A1214"
#define COLOR_CONTROL_CHAR_UUID "19B10003-E8F2-537E-4F6C-D104768A1214"
#define HEART_RATE_RAINBOW_CHAR_UUID "19B10004-E8F2-537E-4F6C-D104768A1214"

// Forward declaration
class AnimationManager;

// Callback class for mode control characteristic writes
class ModeControlCallbacks : public BLECharacteristicCallbacks
{
public:
  ModeControlCallbacks(AnimationMode *currentMode, bool *modeChanged);
  void onWrite(BLECharacteristic *pCharacteristic) override;

private:
  AnimationMode *m_currentMode;
  bool *m_modeChanged;
};

// Callback class for color control characteristic writes
class ColorControlCallbacks : public BLECharacteristicCallbacks
{
public:
  ColorControlCallbacks(AnimationManager *animationManager);
  void onWrite(BLECharacteristic *pCharacteristic) override;

private:
  AnimationManager *m_animationManager;
};

// Heart rate pulse: enable/disable slow rainbow hue cycle (1 byte: 0 = off, non-zero = on)
class HeartRateRainbowCallbacks : public BLECharacteristicCallbacks
{
public:
  explicit HeartRateRainbowCallbacks(AnimationManager *animationManager);
  void onWrite(BLECharacteristic *pCharacteristic) override;

private:
  AnimationManager *m_animationManager;
};

// Callback class for server events
class ControlServerCallbacks : public BLEServerCallbacks
{
public:
  ControlServerCallbacks(bool *deviceConnected);
  void onConnect(BLEServer *pServer) override;
  void onDisconnect(BLEServer *pServer) override;

private:
  bool *m_deviceConnected;
};

class BLEControlService
{
public:
  BLEControlService();
  BLEControlService(class AnimationManager *animationManager);
  ~BLEControlService();

  // Initialization
  bool begin();
  bool isInitialized() const { return m_initialized; }

  // Connection status
  bool isConnected() const { return m_deviceConnected; }

  // Mode management
  void setCurrentMode(AnimationMode mode);
  void notifyCurrentMode(); // Force notification of current mode
  AnimationMode getCurrentMode() const { return m_currentMode; }
  bool hasModeChanged() const { return m_modeChanged; }
  void clearModeChangedFlag() { m_modeChanged = false; }

  // Update - call this in loop() to handle notifications
  void update();

private:
  BLEServer *m_pServer;
  BLEService *m_pService;
  BLECharacteristic *m_pModeControlChar;
  BLECharacteristic *m_pModeStatusChar;
  BLECharacteristic *m_pColorControlChar;
  BLECharacteristic *m_pHeartRateRainbowChar;

  bool m_initialized;
  bool m_deviceConnected;
  bool m_oldDeviceConnected;

  AnimationMode m_currentMode;
  bool m_modeChanged;
  AnimationManager *m_animationManager;

  void setupService();
  void notifyModeStatus();
};

#endif // BLE_CONTROL_SERVICE_H
