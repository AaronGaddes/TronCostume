// BLE Service UUIDs
export const CONTROL_SERVICE_UUID = "19b10000-e8f2-537e-4f6c-d104768a1214";
export const MODE_CONTROL_CHAR_UUID = "19b10001-e8f2-537e-4f6c-d104768a1214";
export const MODE_STATUS_CHAR_UUID = "19b10002-e8f2-537e-4f6c-d104768a1214";

// Device name
export const DEVICE_NAME = "ESP32-LED-Controller";

// Animation modes
export const ANIMATION_MODES = {
  MODE_OFF: 0,
  MODE_SOLID: 1,
  MODE_RAINBOW: 2,
  MODE_HEART_RATE_PULSE: 3,
};
export type AnimationMode =
  (typeof ANIMATION_MODES)[keyof typeof ANIMATION_MODES];
export const MODES = Object.values(ANIMATION_MODES);
// Connection state
export type ConnectionState =
  | "disconnected"
  | "connecting"
  | "connected"
  | "error";

// Bluetooth service interface
export interface BluetoothService {
  connect(): Promise<void>;
  disconnect(): void;
  setMode(mode: AnimationMode): Promise<void>;
  getCurrentMode(): Promise<AnimationMode>;
  isConnected(): boolean;
  getConnectionState(): ConnectionState;
  onConnectionStateChange(callback: (state: ConnectionState) => void): void;
  onModeChange(callback: (mode: AnimationMode) => void): void;
}
