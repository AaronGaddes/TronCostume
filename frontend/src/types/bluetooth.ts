// BLE Service UUIDs
export const CONTROL_SERVICE_UUID = "19b10000-e8f2-537e-4f6c-d104768a1214";
export const MODE_CONTROL_CHAR_UUID = "19b10001-e8f2-537e-4f6c-d104768a1214";
export const MODE_STATUS_CHAR_UUID = "19b10002-e8f2-537e-4f6c-d104768a1214";
export const COLOR_CONTROL_CHAR_UUID = "19b10003-e8f2-537e-4f6c-d104768a1214";
export const HEART_RATE_RAINBOW_CHAR_UUID =
  "19b10004-e8f2-537e-4f6c-d104768a1214";
export const HEART_RATE_RAINBOW_IN_PULSE_CHAR_UUID =
  "19b10005-e8f2-537e-4f6c-d104768a1214";

// Device name
export const DEVICE_NAME = "ESP32-LED-Controller";

// Animation modes
export const ANIMATION_MODES = {
  MODE_OFF: 0,
  MODE_SOLID: 1,
  MODE_RAINBOW: 2,
  MODE_BREATHING: 3,
  MODE_CHASE: 4,
  MODE_TWINKLE: 5,
  MODE_FIRE: 6,
  MODE_COLOR_WAVE: 7,
  MODE_HEART_RATE_PULSE: 8,
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
  setColor(r: number, g: number, b: number): Promise<void>;
  getHeartRateRainbowCycle(): Promise<boolean>;
  setHeartRateRainbowCycle(enabled: boolean): Promise<void>;
  supportsHeartRateRainbowCycle(): boolean;
  getHeartRateRainbowInPulse(): Promise<boolean>;
  setHeartRateRainbowInPulse(enabled: boolean): Promise<void>;
  supportsHeartRateRainbowInPulse(): boolean;
  isConnected(): boolean;
  getConnectionState(): ConnectionState;
  onConnectionStateChange(callback: (state: ConnectionState) => void): void;
  onModeChange(callback: (mode: AnimationMode) => void): void;
}
