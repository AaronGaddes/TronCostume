/// <reference types="web-bluetooth" />
import {
  CONTROL_SERVICE_UUID,
  MODE_CONTROL_CHAR_UUID,
  MODE_STATUS_CHAR_UUID,
  COLOR_CONTROL_CHAR_UUID,
  HEART_RATE_RAINBOW_CHAR_UUID,
  HEART_RATE_RAINBOW_IN_PULSE_CHAR_UUID,
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
  private colorControlChar: BluetoothRemoteGATTCharacteristic | null = null;
  private heartRateRainbowChar: BluetoothRemoteGATTCharacteristic | null = null;
  private heartRateRainbowInPulseChar: BluetoothRemoteGATTCharacteristic | null =
    null;
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
      this.colorControlChar = await this.service.getCharacteristic(
        COLOR_CONTROL_CHAR_UUID
      );

      try {
        this.heartRateRainbowChar = await this.service.getCharacteristic(
          HEART_RATE_RAINBOW_CHAR_UUID
        );
      } catch {
        this.heartRateRainbowChar = null;
        console.warn(
          "Heart rate rainbow characteristic unavailable (update device firmware?)"
        );
      }

      try {
        this.heartRateRainbowInPulseChar = await this.service.getCharacteristic(
          HEART_RATE_RAINBOW_IN_PULSE_CHAR_UUID
        );
      } catch {
        this.heartRateRainbowInPulseChar = null;
        console.warn(
          "Heart rate rainbow-in-pulse characteristic unavailable (update device firmware?)"
        );
      }

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
    this.colorControlChar = null;
    this.heartRateRainbowChar = null;
    this.heartRateRainbowInPulseChar = null;
  }

  async setMode(mode: AnimationMode): Promise<void> {
    if (!this.modeControlChar) {
      throw new Error("Not connected to device");
    }

    if (mode < 0 || mode > 8) {
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

  async getHeartRateRainbowCycle(): Promise<boolean> {
    if (!this.heartRateRainbowChar) {
      return false;
    }

    try {
      const value = await this.heartRateRainbowChar.readValue();
      return value.getUint8(0) !== 0;
    } catch (error) {
      console.error("Failed to read heart rate rainbow option:", error);
      throw new Error("Failed to read heart rate rainbow option from device");
    }
  }

  async setHeartRateRainbowCycle(enabled: boolean): Promise<void> {
    if (!this.heartRateRainbowChar) {
      throw new Error(
        "This device firmware does not support heart rate rainbow mode"
      );
    }

    try {
      const data = new Uint8Array([enabled ? 1 : 0]);
      await this.heartRateRainbowChar.writeValue(data);
    } catch (error) {
      console.error("Failed to set heart rate rainbow option:", error);
      throw new Error("Failed to set heart rate rainbow option on device");
    }
  }

  supportsHeartRateRainbowCycle(): boolean {
    return this.heartRateRainbowChar !== null;
  }

  async getHeartRateRainbowInPulse(): Promise<boolean> {
    if (!this.heartRateRainbowInPulseChar) {
      return false;
    }

    try {
      const value = await this.heartRateRainbowInPulseChar.readValue();
      return value.getUint8(0) !== 0;
    } catch (error) {
      console.error("Failed to read heart rate rainbow-in-pulse option:", error);
      throw new Error(
        "Failed to read heart rate rainbow-in-pulse option from device"
      );
    }
  }

  async setHeartRateRainbowInPulse(enabled: boolean): Promise<void> {
    if (!this.heartRateRainbowInPulseChar) {
      throw new Error(
        "This device firmware does not support rainbow within the pulse"
      );
    }

    try {
      const data = new Uint8Array([enabled ? 1 : 0]);
      await this.heartRateRainbowInPulseChar.writeValue(data);
    } catch (error) {
      console.error("Failed to set heart rate rainbow-in-pulse option:", error);
      throw new Error(
        "Failed to set heart rate rainbow-in-pulse option on device"
      );
    }
  }

  supportsHeartRateRainbowInPulse(): boolean {
    return this.heartRateRainbowInPulseChar !== null;
  }

  async setColor(r: number, g: number, b: number): Promise<void> {
    if (!this.colorControlChar) {
      throw new Error("Not connected to device");
    }

    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
      throw new Error(`Invalid color values: RGB(${r}, ${g}, ${b})`);
    }

    try {
      const data = new Uint8Array([r, g, b]);
      await this.colorControlChar.writeValue(data);
    } catch (error) {
      console.error("Failed to set color:", error);
      throw new Error("Failed to set color on device");
    }
  }

  private async readCurrentMode(): Promise<void> {
    if (!this.modeStatusChar) {
      return;
    }

    try {
      const value = await this.modeStatusChar.readValue();
      const mode = value.getUint8(0);
      if (mode >= 0 && mode <= 8) {
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
      if (mode >= 0 && mode <= 8) {
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
