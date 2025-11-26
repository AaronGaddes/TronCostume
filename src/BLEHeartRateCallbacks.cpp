#include "BLEHeartRateCallbacks.h"
#include "BLEHeartRateTypes.h"
#include <Arduino.h>

// Scan callback implementation
MyAdvertisedDeviceCallbacks::MyAdvertisedDeviceCallbacks(
    std::vector<DiscoveredDevice> &devices, BLEUUID heartRateServiceUUID)
    : m_discoveredDevices(devices), m_heartRateServiceUUID(heartRateServiceUUID)
{
}

void MyAdvertisedDeviceCallbacks::onResult(BLEAdvertisedDevice advertisedDevice)
{
  // Check if device advertises Heart Rate Service or is a Garmin device
  if (advertisedDevice.isAdvertisingService(m_heartRateServiceUUID) ||
      (advertisedDevice.haveName() &&
       (advertisedDevice.getName().find("Garmin") != std::string::npos ||
        advertisedDevice.getName().find("garmin") != std::string::npos)))
  {

    // Check if we already have this device
    bool found = false;
    for (auto &dev : m_discoveredDevices)
    {
      if (dev.address.equals(advertisedDevice.getAddress()))
      {
        found = true;
        // Update RSSI if better
        if (advertisedDevice.getRSSI() > dev.rssi)
        {
          dev.rssi = advertisedDevice.getRSSI();
        }
        break;
      }
    }

    // Add new device
    if (!found)
    {
      std::string name = advertisedDevice.haveName() ? advertisedDevice.getName() : "Unknown";
      m_discoveredDevices.push_back(DiscoveredDevice(
          advertisedDevice.getAddress(),
          name,
          advertisedDevice.getRSSI()));
    }
  }
}

// Client callback implementation
MyClientCallback::MyClientCallback(bool &connected, BLERemoteCharacteristic *&heartRateChar)
    : m_deviceConnected(connected), m_pHeartRateCharacteristic(heartRateChar)
{
}

void MyClientCallback::onConnect(BLEClient *pclient)
{
  Serial.println("Connected to device");
  m_deviceConnected = true;
}

void MyClientCallback::onDisconnect(BLEClient *pclient)
{
  Serial.println("Disconnected from device");
  m_deviceConnected = false;
  m_pHeartRateCharacteristic = nullptr;
}

// Heart rate notification callback implementation
void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                    uint8_t *pData, size_t length, bool isNotify,
                    uint16_t &currentHeartRate)
{
  if (length < 2)
  {
    return;
  }

  // Parse heart rate data according to BLE Heart Rate Measurement format
  // First byte: flags
  // Bit 0: 0 = 8-bit heart rate, 1 = 16-bit heart rate
  uint8_t flags = pData[0];
  bool is16Bit = (flags & 0x01) != 0;

  if (is16Bit && length >= 3)
  {
    // 16-bit heart rate value
    currentHeartRate = (uint16_t)pData[1] | ((uint16_t)pData[2] << 8);
  }
  else if (!is16Bit && length >= 2)
  {
    // 8-bit heart rate value
    currentHeartRate = (uint16_t)pData[1];
  }

  Serial.print("Heart Rate: ");
  Serial.print(currentHeartRate);
  Serial.println(" bpm");
}

