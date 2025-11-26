#ifndef BLE_HEART_RATE_TYPES_H
#define BLE_HEART_RATE_TYPES_H

#include <BLEAddress.h>
#include <string>

// BLE UUIDs for Heart Rate Service
#define HEART_RATE_SERVICE_UUID BLEUUID((uint16_t)0x180D)
#define HEART_RATE_MEASUREMENT_UUID BLEUUID((uint16_t)0x2A37)

// Structure to store discovered devices
struct DiscoveredDevice
{
  BLEAddress address;
  std::string name;
  int rssi;

  DiscoveredDevice() : address((uint8_t *)"\0\0\0\0\0\0"), name(""), rssi(0) {}
  DiscoveredDevice(BLEAddress addr, std::string n, int r) : address(addr), name(n), rssi(r) {}
};

#endif // BLE_HEART_RATE_TYPES_H
