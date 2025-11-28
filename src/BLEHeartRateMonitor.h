#ifndef BLE_HEART_RATE_MONITOR_H
#define BLE_HEART_RATE_MONITOR_H

#include "BLEHeartRateTypes.h"
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLERemoteCharacteristic.h>
#include <vector>

class BLEHeartRateMonitor
{
public:
  BLEHeartRateMonitor();
  ~BLEHeartRateMonitor();

  // Initialization
  bool begin();
  bool isInitialized() const { return m_bleInitialized; }
  void shutdown(); // Deinitialize BLE when not needed

  // Device discovery
  bool scanForDevices(uint32_t scanDurationSeconds = 5);
  void displayDiscoveredDevices() const;
  size_t getDiscoveredDeviceCount() const { return m_discoveredDevices.size(); }

  // Connection management
  bool connectToDevice(int deviceIndex);
  bool connectToFirstHeartRateDevice(); // Automatically connect to first device with heart rate service
  bool isConnected() const { return m_deviceConnected; }
  void disconnect();

  // Heart rate data
  uint16_t getCurrentHeartRate() const { return m_currentHeartRate; }
  bool hasNewHeartRate() const { return m_hasNewHeartRate; }
  void clearNewHeartRateFlag() { m_hasNewHeartRate = false; }

  // State management
  void update(); // Call this in loop()
  void handleDisconnection();
  bool shouldRescan() const;

  // Configuration
  void setScanInterval(unsigned long intervalMs) { m_scanInterval = intervalMs; }
  unsigned long getScanInterval() const { return m_scanInterval; }

private:
  // BLE objects
  BLEScan *m_pBLEScan;
  BLEClient *m_pClient;
  BLERemoteCharacteristic *m_pHeartRateCharacteristic;

  // State
  bool m_bleInitialized;
  bool m_deviceConnected;
  bool m_oldDeviceConnected;

  // Device discovery
  std::vector<DiscoveredDevice> m_discoveredDevices;

  // Timing
  unsigned long m_lastScanTime;
  unsigned long m_scanInterval;

  // Internal methods
  void initializeBLE();
  void setupScan();
  bool discoverHeartRateService();
  void cleanupConnection();

  // Friend function for notification callback
  friend void heartRateNotifyWrapper(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                                     uint8_t *pData, size_t length, bool isNotify);

  // Static instance pointer for callback access
  static BLEHeartRateMonitor *s_instance;

  // Make these accessible to friend function
  uint16_t m_currentHeartRate;
  bool m_hasNewHeartRate;
};

#endif // BLE_HEART_RATE_MONITOR_H
