#include "BLEControlService.h"
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

// BLEControlService implementation
BLEControlService::BLEControlService()
    : m_pServer(nullptr),
      m_pService(nullptr),
      m_pModeControlChar(nullptr),
      m_pModeStatusChar(nullptr),
      m_initialized(false),
      m_deviceConnected(false),
      m_oldDeviceConnected(false),
      m_currentMode(MODE_OFF),
      m_modeChanged(false)
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
