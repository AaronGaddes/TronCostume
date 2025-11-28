#include "BLEHeartRateMonitor.h"
#include "BLEHeartRateCallbacks.h"
#include <Arduino.h>

// Static instance pointer for callback access
BLEHeartRateMonitor *BLEHeartRateMonitor::s_instance = nullptr;

// Forward declaration of wrapper function
void heartRateNotifyWrapper(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                            uint8_t *pData, size_t length, bool isNotify);

BLEHeartRateMonitor::BLEHeartRateMonitor()
    : m_pBLEScan(nullptr),
      m_pClient(nullptr),
      m_pHeartRateCharacteristic(nullptr),
      m_bleInitialized(false),
      m_deviceConnected(false),
      m_oldDeviceConnected(false),
      m_currentHeartRate(0),
      m_hasNewHeartRate(false),
      m_lastScanTime(0),
      m_scanInterval(10000) // Default 10 seconds
{
  s_instance = this;
}

BLEHeartRateMonitor::~BLEHeartRateMonitor()
{
  disconnect();
  if (s_instance == this)
  {
    s_instance = nullptr;
  }
}

bool BLEHeartRateMonitor::begin()
{
  if (m_bleInitialized)
  {
    return true; // Already initialized
  }

  Serial.println("BLE Heart Rate Monitor");
  Serial.println("======================");

  initializeBLE();
  setupScan();
  m_bleInitialized = true;

  return true;
}

void BLEHeartRateMonitor::initializeBLE()
{
  // Only initialize if not already initialized (might be initialized by BLEControlService)
  if (!BLEDevice::getInitialized())
  {
    BLEDevice::init("");
  }
}

void BLEHeartRateMonitor::setupScan()
{
  m_pBLEScan = BLEDevice::getScan();
  m_pBLEScan->setAdvertisedDeviceCallbacks(
      new MyAdvertisedDeviceCallbacks(m_discoveredDevices, HEART_RATE_SERVICE_UUID));
  m_pBLEScan->setActiveScan(true); // Active scan uses more power but gets more results
  m_pBLEScan->setInterval(100);
  m_pBLEScan->setWindow(99);
}

bool BLEHeartRateMonitor::scanForDevices(uint32_t scanDurationSeconds)
{
  if (!m_bleInitialized)
  {
    Serial.println("ERROR: BLE not initialized. Call begin() first.");
    return false;
  }

  Serial.println("Scanning for BLE devices...");
  m_discoveredDevices.clear();

  BLEScanResults foundDevices = m_pBLEScan->start(scanDurationSeconds, false);
  m_pBLEScan->clearResults();

  Serial.print("Found ");
  Serial.print(m_discoveredDevices.size());
  Serial.println(" device(s) with Heart Rate Service:");

  if (m_discoveredDevices.size() == 0)
  {
    Serial.println("No devices found. Make sure your Garmin watch is nearby and broadcasting.");
    return false;
  }

  displayDiscoveredDevices();
  m_lastScanTime = millis();
  return true;
}

void BLEHeartRateMonitor::displayDiscoveredDevices() const
{
  for (size_t i = 0; i < m_discoveredDevices.size(); i++)
  {
    Serial.print(i + 1);
    Serial.print(". ");
    Serial.print(m_discoveredDevices[i].name.c_str());
    Serial.print(" (");
    // Make a non-const copy to call toString()
    BLEAddress addr = m_discoveredDevices[i].address;
    Serial.print(addr.toString().c_str());
    Serial.print(") - RSSI: ");
    Serial.println(m_discoveredDevices[i].rssi);
  }

  Serial.println("\nEnter device number to connect (or 0 to rescan):");
}

bool BLEHeartRateMonitor::connectToDevice(int deviceIndex)
{
  if (!m_bleInitialized)
  {
    Serial.println("ERROR: BLE not initialized. Call begin() first.");
    return false;
  }

  if (deviceIndex < 1 || deviceIndex > (int)m_discoveredDevices.size())
  {
    Serial.println("Invalid device number");
    return false;
  }

  DiscoveredDevice &selectedDevice = m_discoveredDevices[deviceIndex - 1];

  Serial.print("Connecting to: ");
  Serial.println(selectedDevice.name.c_str());

  // Create BLE client
  m_pClient = BLEDevice::createClient();
  m_pClient->setClientCallbacks(new MyClientCallback(m_deviceConnected, m_pHeartRateCharacteristic));

  // Connect to the device
  if (!m_pClient->connect(selectedDevice.address))
  {
    Serial.println("Failed to connect");
    return false;
  }

  Serial.println("Connected! Discovering services...");

  if (!discoverHeartRateService())
  {
    disconnect();
    return false;
  }

  Serial.println("Subscribed to heart rate notifications");
  Serial.println("Waiting for heart rate data...\n");
  return true;
}

bool BLEHeartRateMonitor::connectToFirstHeartRateDevice()
{
  if (!m_bleInitialized)
  {
    Serial.println("ERROR: BLE not initialized. Call begin() first.");
    return false;
  }

  if (m_deviceConnected)
  {
    Serial.println("Already connected to a device");
    return true;
  }

  Serial.println("Scanning for heart rate device...");

  // Scan for devices (scanForDevices already filters for devices with heart rate service)
  if (!scanForDevices(5)) // 5 second scan
  {
    Serial.println("No heart rate devices found during scan");
    return false;
  }

  // Connect to the first device found (all discovered devices have heart rate service)
  if (m_discoveredDevices.size() == 0)
  {
    Serial.println("No heart rate devices found");
    return false;
  }

  Serial.print("Found heart rate device: ");
  Serial.print(m_discoveredDevices[0].name.c_str());
  Serial.println(", connecting...");
  return connectToDevice(1); // Connect to first device (1-based index)
}

bool BLEHeartRateMonitor::discoverHeartRateService()
{
  // Get Heart Rate Service
  BLERemoteService *pRemoteService = m_pClient->getService(HEART_RATE_SERVICE_UUID);
  if (pRemoteService == nullptr)
  {
    Serial.println("Failed to find Heart Rate Service");
    return false;
  }

  Serial.println("Found Heart Rate Service");

  // Get Heart Rate Measurement characteristic
  m_pHeartRateCharacteristic = pRemoteService->getCharacteristic(HEART_RATE_MEASUREMENT_UUID);
  if (m_pHeartRateCharacteristic == nullptr)
  {
    Serial.println("Failed to find Heart Rate Measurement characteristic");
    return false;
  }

  Serial.println("Found Heart Rate Measurement characteristic");

  // Subscribe to notifications
  if (m_pHeartRateCharacteristic->canNotify())
  {
    m_pHeartRateCharacteristic->registerForNotify(heartRateNotifyWrapper);
    return true;
  }
  else
  {
    Serial.println("Characteristic does not support notifications");
    return false;
  }
}

void BLEHeartRateMonitor::disconnect()
{
  if (m_pClient != nullptr && m_pClient->isConnected())
  {
    m_pClient->disconnect();
  }
  cleanupConnection();
}

void BLEHeartRateMonitor::cleanupConnection()
{
  m_pClient = nullptr;
  m_pHeartRateCharacteristic = nullptr;
  m_deviceConnected = false;
}

void BLEHeartRateMonitor::update()
{
  // Handle disconnection
  if (!m_deviceConnected && m_oldDeviceConnected)
  {
    handleDisconnection();
  }

  // Handle new connection
  if (m_deviceConnected && !m_oldDeviceConnected)
  {
    m_oldDeviceConnected = m_deviceConnected;
  }
}

void BLEHeartRateMonitor::handleDisconnection()
{
  delay(500); // Give the bluetooth stack time to cleanup
  cleanupConnection();
  Serial.println("\nDevice disconnected. Will rescan in 10 seconds...");
  m_lastScanTime = millis();
  m_oldDeviceConnected = m_deviceConnected;
}

bool BLEHeartRateMonitor::shouldRescan() const
{
  if (!m_bleInitialized || m_deviceConnected)
  {
    return false;
  }

  unsigned long currentTime = millis();
  return (m_lastScanTime == 0 || (currentTime - m_lastScanTime >= m_scanInterval));
}

void BLEHeartRateMonitor::shutdown()
{
  if (!m_bleInitialized)
  {
    return; // Already shut down
  }

  disconnect();

  // Note: BLEDevice::deinit() may not be available in all ESP32 BLE libraries
  // We'll just mark as not initialized and clean up resources
  m_pBLEScan = nullptr;
  m_bleInitialized = false;

  Serial.println("BLE Heart Rate Monitor shut down");
}

// Wrapper function for heart rate notification callback
// This matches the BLE library's notify_callback signature
void heartRateNotifyWrapper(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                            uint8_t *pData, size_t length, bool isNotify)
{
  if (BLEHeartRateMonitor::s_instance != nullptr)
  {
    notifyCallback(pBLERemoteCharacteristic, pData, length, isNotify,
                   BLEHeartRateMonitor::s_instance->m_currentHeartRate);
    BLEHeartRateMonitor::s_instance->m_hasNewHeartRate = true;
  }
}
