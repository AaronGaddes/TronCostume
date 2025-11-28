/// <reference types="web-bluetooth" />
import {
  CONTROL_SERVICE_UUID,
  MODE_CONTROL_CHAR_UUID,
  MODE_STATUS_CHAR_UUID,
  DEVICE_NAME,
  ANIMATION_MODES,
  type AnimationMode,
  type ConnectionState,
  type BluetoothService,
} from "@/types/bluetooth";

class BluetoothServiceImpl implements BluetoothService {
  private device: BluetoothDevice | null = null;
  private server: BluetoothRemoteGATTServer | null = null;
  private service: BluetoothRemoteGATTService | null = null;
  private modeControlChar: BluetoothRemoteGATTCharacteristic | null = null;
  private modeStatusChar: BluetoothRemoteGATTCharacteristic | null = null;
  private connectionState: ConnectionState = "disconnected";
  private connectionStateCallbacks: ((state: ConnectionState) => void)[] = [];
  private modeChangeCallbacks: ((mode: AnimationMode) => void)[] = [];
  private currentMode: AnimationMode = ANIMATION_MODES.MODE_OFF;

  constructor() {
    // Check if Web Bluetooth is available
    if (!navigator.bluetooth) {
      console.error("Web Bluetooth API is not available in this browser");
    }
  }

  private setConnectionState(state: ConnectionState) {
    if (this.connectionState !== state) {
      this.connectionState = state;
      this.connectionStateCallbacks.forEach((callback) => callback(state));
    }
  }

  async connect(): Promise<void> {
    if (!navigator.bluetooth) {
      throw new Error("Web Bluetooth API is not available in this browser");
    }

    if (this.connectionState === "connected") {
      return;
    }

    try {
      this.setConnectionState("connecting");

      // Try to request device with filters first (service UUID and name)
      // If that fails, try with acceptAllDevices as fallback
      try {
        this.device = await navigator.bluetooth.requestDevice({
          filters: [
            { services: [CONTROL_SERVICE_UUID] },
            { name: DEVICE_NAME },
          ],
          optionalServices: [CONTROL_SERVICE_UUID],
        });
      } catch (filterError) {
        // If filtered request fails, try showing all devices
        // User can then select the device manually
        console.warn(
          "Filtered device request failed, trying acceptAllDevices:",
          filterError
        );
        this.device = await navigator.bluetooth.requestDevice({
          acceptAllDevices: true,
          optionalServices: [CONTROL_SERVICE_UUID],
        });
      }

      // Set up disconnect handler
      this.device.addEventListener("gattserverdisconnected", () => {
        this.handleDisconnection();
      });

      // Connect to GATT server
      if (!this.device.gatt) {
        throw new Error("GATT server not available");
      }

      this.server = await this.device.gatt.connect();

      // Get the service
      this.service = await this.server.getPrimaryService(CONTROL_SERVICE_UUID);

      // Get characteristics
      this.modeControlChar = await this.service.getCharacteristic(
        MODE_CONTROL_CHAR_UUID
      );
      this.modeStatusChar = await this.service.getCharacteristic(
        MODE_STATUS_CHAR_UUID
      );

      // Subscribe to mode status notifications
      await this.modeStatusChar.startNotifications();
      this.modeStatusChar.addEventListener(
        "characteristicvaluechanged",
        this.handleModeStatusChange.bind(this)
      );

      // Read initial mode
      await this.readCurrentMode();

      this.setConnectionState("connected");
    } catch (error) {
      this.setConnectionState("error");
      if (error instanceof Error) {
        if (error.name === "NotFoundError") {
          throw new Error(
            `Device "${DEVICE_NAME}" not found. Make sure it's powered on and advertising.`
          );
        } else if (error.name === "SecurityError") {
          throw new Error(
            "Bluetooth connection was blocked. Please check browser permissions."
          );
        } else if (error.name === "NetworkError") {
          throw new Error("Connection failed. The device may be out of range.");
        }
      }
      throw error;
    }
  }

  disconnect(): void {
    if (this.modeStatusChar) {
      this.modeStatusChar.stopNotifications().catch(console.error);
    }

    if (this.device?.gatt?.connected) {
      this.device.gatt.disconnect();
    }

    this.cleanup();
    this.setConnectionState("disconnected");
  }

  private handleDisconnection(): void {
    this.cleanup();
    this.setConnectionState("disconnected");
  }

  private cleanup(): void {
    this.server = null;
    this.service = null;
    this.modeControlChar = null;
    this.modeStatusChar = null;
  }

  async setMode(mode: AnimationMode): Promise<void> {
    if (!this.modeControlChar) {
      throw new Error("Not connected to device");
    }

    if (mode < 0 || mode > 3) {
      throw new Error(`Invalid mode: ${mode}`);
    }

    try {
      const data = new Uint8Array([mode]);
      await this.modeControlChar.writeValue(data);
    } catch (error) {
      console.error("Failed to set mode:", error);
      throw new Error("Failed to set mode on device");
    }
  }

  async getCurrentMode(): Promise<AnimationMode> {
    if (!this.modeStatusChar) {
      throw new Error("Not connected to device");
    }

    await this.readCurrentMode();
    return this.currentMode;
  }

  private async readCurrentMode(): Promise<void> {
    if (!this.modeStatusChar) {
      return;
    }

    try {
      const value = await this.modeStatusChar.readValue();
      const mode = value.getUint8(0);
      if (mode >= 0 && mode <= 3) {
        this.currentMode = mode as AnimationMode;
        this.modeChangeCallbacks.forEach((callback) =>
          callback(this.currentMode)
        );
      }
    } catch (error) {
      console.error("Failed to read current mode:", error);
    }
  }

  private handleModeStatusChange(event: Event): void {
    const characteristic = event.target as BluetoothRemoteGATTCharacteristic;
    const value = characteristic.value;
    if (value) {
      const mode = value.getUint8(0);
      if (mode >= 0 && mode <= 3) {
        this.currentMode = mode as AnimationMode;
        this.modeChangeCallbacks.forEach((callback) =>
          callback(this.currentMode)
        );
      }
    }
  }

  isConnected(): boolean {
    return (
      this.connectionState === "connected" &&
      this.device?.gatt?.connected === true
    );
  }

  getConnectionState(): ConnectionState {
    return this.connectionState;
  }

  onConnectionStateChange(callback: (state: ConnectionState) => void): void {
    this.connectionStateCallbacks.push(callback);
  }

  onModeChange(callback: (mode: AnimationMode) => void): void {
    this.modeChangeCallbacks.push(callback);
  }
}

// Export singleton instance
export const bluetoothService = new BluetoothServiceImpl();
