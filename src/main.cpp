#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLEAdvertisedDevice.h>
#include <BLERemoteCharacteristic.h>
#include <BLERemoteService.h>

// BLE UUIDs for Heart Rate Service
#define HEART_RATE_SERVICE_UUID BLEUUID((uint16_t)0x180D)
#define HEART_RATE_MEASUREMENT_UUID BLEUUID((uint16_t)0x2A37)

// Global variables
BLEScan *pBLEScan;
BLEClient *pClient = nullptr;
BLERemoteCharacteristic *pHeartRateCharacteristic = nullptr;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint16_t currentHeartRate = 0;
unsigned long lastScanTime = 0;
const unsigned long SCAN_INTERVAL = 10000; // 10 seconds between scans if disconnected

// Structure to store discovered devices
struct DiscoveredDevice
{
  BLEAddress address;
  std::string name;
  int rssi;

  DiscoveredDevice() : address((uint8_t *)"\0\0\0\0\0\0"), name(""), rssi(0) {}
  DiscoveredDevice(BLEAddress addr, std::string n, int r) : address(addr), name(n), rssi(r) {}
};

std::vector<DiscoveredDevice> discoveredDevices;

// Scan callback class - collects devices advertising Heart Rate Service
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    // Check if device advertises Heart Rate Service or is a Garmin device
    if (advertisedDevice.isAdvertisingService(HEART_RATE_SERVICE_UUID) ||
        (advertisedDevice.haveName() &&
         (advertisedDevice.getName().find("Garmin") != std::string::npos ||
          advertisedDevice.getName().find("garmin") != std::string::npos)))
    {

      // Check if we already have this device
      bool found = false;
      for (auto &dev : discoveredDevices)
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
        discoveredDevices.push_back(DiscoveredDevice(
            advertisedDevice.getAddress(),
            name,
            advertisedDevice.getRSSI()));
      }
    }
  }
};

// Client callback class - handles connection events
class MyClientCallback : public BLEClientCallbacks
{
  void onConnect(BLEClient *pclient)
  {
    Serial.println("Connected to device");
    deviceConnected = true;
  }

  void onDisconnect(BLEClient *pclient)
  {
    Serial.println("Disconnected from device");
    deviceConnected = false;
    pHeartRateCharacteristic = nullptr;
  }
};

// Heart rate notification callback
void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                    uint8_t *pData, size_t length, bool isNotify)
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

// Function to scan for devices
void scanForDevices()
{
  Serial.println("Scanning for BLE devices...");
  discoveredDevices.clear();

  BLEScanResults foundDevices = pBLEScan->start(5, false); // Scan for 5 seconds
  pBLEScan->clearResults();

  Serial.print("Found ");
  Serial.print(discoveredDevices.size());
  Serial.println(" device(s) with Heart Rate Service:");

  if (discoveredDevices.size() == 0)
  {
    Serial.println("No devices found. Make sure your Garmin watch is nearby and broadcasting.");
    return;
  }

  // Display found devices
  for (size_t i = 0; i < discoveredDevices.size(); i++)
  {
    Serial.print(i + 1);
    Serial.print(". ");
    Serial.print(discoveredDevices[i].name.c_str());
    Serial.print(" (");
    Serial.print(discoveredDevices[i].address.toString().c_str());
    Serial.print(") - RSSI: ");
    Serial.println(discoveredDevices[i].rssi);
  }

  Serial.println("\nEnter device number to connect (or 0 to rescan):");
}

// Function to connect to selected device
bool connectToDevice(int deviceIndex)
{
  if (deviceIndex < 1 || deviceIndex > (int)discoveredDevices.size())
  {
    Serial.println("Invalid device number");
    return false;
  }

  DiscoveredDevice &selectedDevice = discoveredDevices[deviceIndex - 1];

  Serial.print("Connecting to: ");
  Serial.println(selectedDevice.name.c_str());

  // Create BLE client
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the device
  if (!pClient->connect(selectedDevice.address))
  {
    Serial.println("Failed to connect");
    return false;
  }

  Serial.println("Connected! Discovering services...");

  // Get Heart Rate Service
  BLERemoteService *pRemoteService = pClient->getService(HEART_RATE_SERVICE_UUID);
  if (pRemoteService == nullptr)
  {
    Serial.println("Failed to find Heart Rate Service");
    pClient->disconnect();
    return false;
  }

  Serial.println("Found Heart Rate Service");

  // Get Heart Rate Measurement characteristic
  pHeartRateCharacteristic = pRemoteService->getCharacteristic(HEART_RATE_MEASUREMENT_UUID);
  if (pHeartRateCharacteristic == nullptr)
  {
    Serial.println("Failed to find Heart Rate Measurement characteristic");
    pClient->disconnect();
    return false;
  }

  Serial.println("Found Heart Rate Measurement characteristic");

  // Subscribe to notifications
  if (pHeartRateCharacteristic->canNotify())
  {
    pHeartRateCharacteristic->registerForNotify(notifyCallback);
    Serial.println("Subscribed to heart rate notifications");
    Serial.println("Waiting for heart rate data...\n");
    return true;
  }
  else
  {
    Serial.println("Characteristic does not support notifications");
    pClient->disconnect();
    return false;
  }
}

// Function to check for user input
int checkUserInput()
{
  if (Serial.available() > 0)
  {
    String input = Serial.readStringUntil('\n');
    input.trim();
    return input.toInt();
  }
  return -1;
}

void setup()
{
  Serial.begin(115200);
  Serial.println("BLE Heart Rate Monitor");
  Serial.println("======================");

  // Initialize BLE
  BLEDevice::init("");

  // Get scan object and set callback
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); // Active scan uses more power but gets more results
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);

  // Initial scan
  scanForDevices();
}

void loop()
{
  // Check if we need to scan (not connected and enough time has passed)
  if (!deviceConnected && !oldDeviceConnected)
  {
    unsigned long currentTime = millis();
    if (currentTime - lastScanTime >= SCAN_INTERVAL || lastScanTime == 0)
    {
      int userInput = checkUserInput();
      if (userInput == -1)
      {
        // No input, check if we should rescan
        if (lastScanTime == 0 || (currentTime - lastScanTime >= SCAN_INTERVAL))
        {
          scanForDevices();
          lastScanTime = currentTime;
        }
      }
      else if (userInput == 0)
      {
        // User wants to rescan
        scanForDevices();
        lastScanTime = currentTime;
      }
      else
      {
        // User selected a device
        connectToDevice(userInput);
        lastScanTime = currentTime;
      }
    }
    else
    {
      // Check for user input while waiting
      int userInput = checkUserInput();
      if (userInput > 0 && userInput <= (int)discoveredDevices.size())
      {
        connectToDevice(userInput);
        lastScanTime = millis();
      }
      else if (userInput == 0)
      {
        scanForDevices();
        lastScanTime = millis();
      }
    }
  }

  // Handle disconnection
  if (!deviceConnected && oldDeviceConnected)
  {
    delay(500); // Give the bluetooth stack time to cleanup
    pClient = nullptr;
    pHeartRateCharacteristic = nullptr;
    Serial.println("\nDevice disconnected. Will rescan in 10 seconds...");
    lastScanTime = millis();
    oldDeviceConnected = deviceConnected;
  }

  // Handle new connection
  if (deviceConnected && !oldDeviceConnected)
  {
    oldDeviceConnected = deviceConnected;
  }

  delay(100); // Small delay to prevent tight loop
}
