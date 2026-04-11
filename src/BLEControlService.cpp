#include "BLEControlService.h"
#include "AnimationManager.h"
#include <Arduino.h>

// ModeControlCallbacks implementation
ModeControlCallbacks::ModeControlCallbacks(AnimationMode *currentMode, bool *modeChanged)
    : m_currentMode(currentMode), m_modeChanged(modeChanged)
{
}

void ModeControlCallbacks::onWrite(BLECharacteristic *pCharacteristic)
{
  std::string value = pCharacteristic->getValue();

  if (value.length() == 1)
  {
    uint8_t modeValue = value[0];

    // Validate mode value
    if (modeValue < MODE_COUNT)
    {
      *m_currentMode = (AnimationMode)modeValue;
      *m_modeChanged = true;

      Serial.print("BLE: Mode changed to ");
      Serial.print(getModeName(*m_currentMode));
      Serial.print(" (");
      Serial.print(modeValue);
      Serial.println(")");
    }
    else
    {
      Serial.print("BLE: Invalid mode value received: ");
      Serial.println(modeValue);
    }
  }
  else
  {
    Serial.print("BLE: Invalid data length received: ");
    Serial.println(value.length());
  }
}

// ControlServerCallbacks implementation
ControlServerCallbacks::ControlServerCallbacks(bool *deviceConnected)
    : m_deviceConnected(deviceConnected)
{
}

void ControlServerCallbacks::onConnect(BLEServer *pServer)
{
  *m_deviceConnected = true;
  Serial.println("BLE: Client connected");
}

void ControlServerCallbacks::onDisconnect(BLEServer *pServer)
{
  *m_deviceConnected = false;
  Serial.println("BLE: Client disconnected");

  // Restart advertising
  BLEDevice::startAdvertising();
  Serial.println("BLE: Advertising restarted");
}

// ColorControlCallbacks implementation
ColorControlCallbacks::ColorControlCallbacks(AnimationManager *animationManager)
    : m_animationManager(animationManager)
{
}

void ColorControlCallbacks::onWrite(BLECharacteristic *pCharacteristic)
{
  if (m_animationManager == nullptr)
  {
    return;
  }

  std::string value = pCharacteristic->getValue();

  // Expect 3 bytes: R, G, B
  if (value.length() == 3)
  {
    uint8_t r = value[0];
    uint8_t g = value[1];
    uint8_t b = value[2];

    m_animationManager->setSolidColor(r, g, b);

    Serial.print("BLE: Color changed to RGB(");
    Serial.print(r);
    Serial.print(", ");
    Serial.print(g);
    Serial.print(", ");
    Serial.print(b);
    Serial.println(")");
  }
  else
  {
    Serial.print("BLE: Invalid color data length received: ");
    Serial.println(value.length());
  }
}

HeartRateRainbowCallbacks::HeartRateRainbowCallbacks(AnimationManager *animationManager)
    : m_animationManager(animationManager)
{
}

void HeartRateRainbowCallbacks::onWrite(BLECharacteristic *pCharacteristic)
{
  if (m_animationManager == nullptr || pCharacteristic == nullptr)
  {
    return;
  }

  std::string value = pCharacteristic->getValue();
  if (value.length() != 1)
  {
    Serial.print("BLE: Invalid heart rate rainbow data length: ");
    Serial.println(value.length());
    return;
  }

  bool enabled = value[0] != 0;
  m_animationManager->setHeartRateRainbowCycle(enabled);
  uint8_t stored = enabled ? 1 : 0;
  pCharacteristic->setValue(&stored, 1);
}

HeartRateRainbowInPulseCallbacks::HeartRateRainbowInPulseCallbacks(
    AnimationManager *animationManager)
    : m_animationManager(animationManager)
{
}

void HeartRateRainbowInPulseCallbacks::onWrite(BLECharacteristic *pCharacteristic)
{
  if (m_animationManager == nullptr || pCharacteristic == nullptr)
  {
    return;
  }

  std::string value = pCharacteristic->getValue();
  if (value.length() != 1)
  {
    Serial.print("BLE: Invalid heart rate rainbow-in-pulse data length: ");
    Serial.println(value.length());
    return;
  }

  bool enabled = value[0] != 0;
  m_animationManager->setHeartRateRainbowInPulse(enabled);
  uint8_t stored = enabled ? 1 : 0;
  pCharacteristic->setValue(&stored, 1);
}

// BLEControlService implementation
BLEControlService::BLEControlService()
    : BLEControlService(nullptr)
{
}

BLEControlService::BLEControlService(AnimationManager *animationManager)
    : m_pServer(nullptr),
      m_pService(nullptr),
      m_pModeControlChar(nullptr),
      m_pModeStatusChar(nullptr),
      m_pColorControlChar(nullptr),
      m_pHeartRateRainbowChar(nullptr),
      m_pHeartRateRainbowInPulseChar(nullptr),
      m_initialized(false),
      m_deviceConnected(false),
      m_oldDeviceConnected(false),
      m_currentMode(MODE_OFF),
      m_modeChanged(false),
      m_animationManager(animationManager)
{
}

BLEControlService::~BLEControlService()
{
  if (m_pServer)
  {
    m_pServer->getAdvertising()->stop();
  }
}

bool BLEControlService::begin()
{
  if (m_initialized)
  {
    return true; // Already initialized
  }

  // Check if BLE is already initialized (might be by BLEHeartRateMonitor)
  // If not, initialize it
  if (!BLEDevice::getInitialized())
  {
    BLEDevice::init(DEVICE_NAME);
    Serial.println("BLE: Initialized BLEDevice");
  }
  else
  {
    // BLE already initialized - device name should already be set
    // If it wasn't set, we can't change it after init, but the service UUID
    // in the advertisement should be sufficient for discovery
    Serial.println("BLE: BLEDevice already initialized");
    Serial.println("Note: Device name may not match if BLE was initialized elsewhere");
  }

  // Create BLE Server
  m_pServer = BLEDevice::createServer();
  m_pServer->setCallbacks(new ControlServerCallbacks(&m_deviceConnected));

  setupService();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  // Add service UUID to advertisement (required for Web Bluetooth discovery)
  pAdvertising->addServiceUUID(CONTROL_SERVICE_UUID);
  // Set advertisement properties
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // Functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  m_initialized = true;
  Serial.println("BLE Control Service: Started advertising");
  Serial.println("Device name: " + String(DEVICE_NAME));

  return true;
}

void BLEControlService::setupService()
{
  // Create the BLE Service
  m_pService = m_pServer->createService(CONTROL_SERVICE_UUID);

  // Create Mode Control Characteristic (write)
  m_pModeControlChar = m_pService->createCharacteristic(
      MODE_CONTROL_CHAR_UUID,
      BLECharacteristic::PROPERTY_WRITE);

  m_pModeControlChar->setCallbacks(
      new ModeControlCallbacks(&m_currentMode, &m_modeChanged));

  // Create Mode Status Characteristic (read, notify)
  m_pModeStatusChar = m_pService->createCharacteristic(
      MODE_STATUS_CHAR_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

  // Add descriptor for notifications
  m_pModeStatusChar->addDescriptor(new BLE2902());

  // Create Color Control Characteristic (write) - for solid mode color
  m_pColorControlChar = m_pService->createCharacteristic(
      COLOR_CONTROL_CHAR_UUID,
      BLECharacteristic::PROPERTY_WRITE);

  if (m_animationManager != nullptr)
  {
    m_pColorControlChar->setCallbacks(
        new ColorControlCallbacks(m_animationManager));
  }

  m_pHeartRateRainbowChar = m_pService->createCharacteristic(
      HEART_RATE_RAINBOW_CHAR_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  if (m_animationManager != nullptr)
  {
    m_pHeartRateRainbowChar->setCallbacks(
        new HeartRateRainbowCallbacks(m_animationManager));
  }

  uint8_t rainbowOff = 0;
  m_pHeartRateRainbowChar->setValue(&rainbowOff, 1);

  m_pHeartRateRainbowInPulseChar = m_pService->createCharacteristic(
      HEART_RATE_RAINBOW_IN_PULSE_CHAR_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  if (m_animationManager != nullptr)
  {
    m_pHeartRateRainbowInPulseChar->setCallbacks(
        new HeartRateRainbowInPulseCallbacks(m_animationManager));
  }

  uint8_t inPulseOff = 0;
  m_pHeartRateRainbowInPulseChar->setValue(&inPulseOff, 1);

  // Start the service
  m_pService->start();

  // Set initial mode status
  uint8_t modeValue = (uint8_t)m_currentMode;
  m_pModeStatusChar->setValue(&modeValue, 1);

  Serial.println("BLE Control Service: Service and characteristics created");
}

void BLEControlService::setCurrentMode(AnimationMode mode)
{
  if (m_currentMode != mode)
  {
    m_currentMode = mode;
    notifyModeStatus();
  }
}

void BLEControlService::notifyCurrentMode()
{
  // Force notification of current mode (useful after processing mode changes)
  notifyModeStatus();
}

void BLEControlService::notifyModeStatus()
{
  if (m_pModeStatusChar && m_deviceConnected)
  {
    uint8_t modeValue = (uint8_t)m_currentMode;
    // Always update the characteristic value before notifying
    // This ensures the value is current even if it hasn't "changed"
    m_pModeStatusChar->setValue(&modeValue, 1);
    m_pModeStatusChar->notify();
  }
}

void BLEControlService::update()
{
  // Handle disconnection
  if (!m_deviceConnected && m_oldDeviceConnected)
  {
    m_oldDeviceConnected = m_deviceConnected;
    delay(500); // Give the bluetooth stack time to cleanup
  }

  // Handle new connection
  if (m_deviceConnected && !m_oldDeviceConnected)
  {
    m_oldDeviceConnected = m_deviceConnected;
    // Send current mode status to newly connected client
    notifyModeStatus();
  }
}
