#ifndef BLE_HEART_RATE_CALLBACKS_H
#define BLE_HEART_RATE_CALLBACKS_H

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLEAdvertisedDevice.h>
#include <BLERemoteCharacteristic.h>
#include <vector>
#include <string>

// Forward declarations
struct DiscoveredDevice;

// Scan callback class - collects devices advertising Heart Rate Service
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
public:
  MyAdvertisedDeviceCallbacks(std::vector<DiscoveredDevice> &devices, BLEUUID heartRateServiceUUID);
  void onResult(BLEAdvertisedDevice advertisedDevice) override;

private:
  std::vector<DiscoveredDevice> &m_discoveredDevices;
  BLEUUID m_heartRateServiceUUID;
};

// Client callback class - handles connection events
class MyClientCallback : public BLEClientCallbacks
{
public:
  MyClientCallback(bool &connected, BLERemoteCharacteristic *&heartRateChar);
  void onConnect(BLEClient *pclient) override;
  void onDisconnect(BLEClient *pclient) override;

private:
  bool &m_deviceConnected;
  BLERemoteCharacteristic *&m_pHeartRateCharacteristic;
};

// Heart rate notification callback function
void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                    uint8_t *pData, size_t length, bool isNotify,
                    uint16_t &currentHeartRate);

#endif // BLE_HEART_RATE_CALLBACKS_H

