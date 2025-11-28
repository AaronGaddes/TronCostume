import { useState } from "react";
import { useBluetooth } from "@/hooks/useBluetooth";
import { type AnimationMode, ANIMATION_MODES, MODES } from "@/types/bluetooth";
import { Button } from "@/components/ui/button";

const MODE_NAMES: Record<AnimationMode, string> = {
  [ANIMATION_MODES.MODE_OFF]: "Off",
  [ANIMATION_MODES.MODE_SOLID]: "Solid",
  [ANIMATION_MODES.MODE_RAINBOW]: "Rainbow",
  [ANIMATION_MODES.MODE_HEART_RATE_PULSE]: "Heart Rate Pulse",
};

export function BluetoothController() {
  const {
    connectionState,
    currentMode,
    error,
    connect,
    disconnect,
    setMode,
    setColor,
    isConnected,
  } = useBluetooth();

  const [isConnecting, setIsConnecting] = useState(false);
  const [isSettingMode, setIsSettingMode] = useState(false);

  const handleConnect = async () => {
    setIsConnecting(true);
    try {
      await connect();
    } catch (err) {
      console.error("Failed to connect:", err);
      // Error is handled by the hook
    } finally {
      setIsConnecting(false);
    }
  };

  const handleDisconnect = () => {
    disconnect();
  };

  const handleModeSelect = async (mode: AnimationMode) => {
    if (!isConnected || mode === currentMode) {
      return;
    }

    setIsSettingMode(true);
    try {
      await setMode(mode);
    } catch (error) {
      console.error("Failed to set mode:", error);
      // Error is handled by the hook
    } finally {
      setIsSettingMode(false);
    }
  };

  const getConnectionStatusText = () => {
    switch (connectionState) {
      case "disconnected":
        return "Disconnected";
      case "connecting":
        return "Connecting...";
      case "connected":
        return "Connected";
      case "error":
        return "Error";
      default:
        return "Unknown";
    }
  };

  const getConnectionStatusColor = () => {
    switch (connectionState) {
      case "connected":
        return "text-green-600 dark:text-green-400";
      case "connecting":
        return "text-yellow-600 dark:text-yellow-400";
      case "error":
        return "text-red-600 dark:text-red-400";
      default:
        return "text-gray-600 dark:text-gray-400";
    }
  };

  return (
    <div className="container mx-auto p-6 max-w-2xl">
      <div className="space-y-6">
        <div>
          <h1 className="text-3xl font-bold mb-2">
            Tron Costume LED Controller
          </h1>
          <p className="text-muted-foreground">
            Connect to your ESP32 device to control LED animation modes
          </p>
        </div>

        {/* Connection Section */}
        <div className="border rounded-lg p-4 space-y-4">
          <div className="flex items-center justify-between">
            <div>
              <h2 className="text-lg font-semibold">Connection</h2>
              <p className={`text-sm ${getConnectionStatusColor()}`}>
                {getConnectionStatusText()}
              </p>
            </div>
            {isConnected ? (
              <Button onClick={handleDisconnect} variant="destructive">
                Disconnect
              </Button>
            ) : (
              <Button
                onClick={handleConnect}
                disabled={isConnecting || connectionState === "connecting"}
              >
                {isConnecting || connectionState === "connecting"
                  ? "Connecting..."
                  : "Connect"}
              </Button>
            )}
          </div>

          {error && (
            <div className="bg-destructive/10 border border-destructive/20 rounded-md p-3">
              <p className="text-sm text-destructive">{error}</p>
            </div>
          )}
        </div>

        {/* Current Mode Display */}
        {isConnected && (
          <div className="border rounded-lg p-4">
            <h2 className="text-lg font-semibold mb-2">Current Mode</h2>
            <p className="text-2xl font-bold">{MODE_NAMES[currentMode]}</p>
          </div>
        )}

        {/* Mode Selection */}
        {isConnected && (
          <div className="border rounded-lg p-4 space-y-4">
            <h2 className="text-lg font-semibold">Select Mode</h2>
            <div className="grid grid-cols-1 sm:grid-cols-2 gap-3">
              {MODES.map((mode) => (
                <Button
                  key={mode}
                  onClick={() => handleModeSelect(mode)}
                  disabled={isSettingMode || currentMode === mode}
                  variant={currentMode === mode ? "default" : "outline"}
                  className="w-full"
                >
                  {MODE_NAMES[mode]}
                  {currentMode === mode && " ✓"}
                </Button>
              ))}
            </div>
            {isSettingMode && (
              <p className="text-sm text-muted-foreground text-center">
                Setting mode...
              </p>
            )}
          </div>
        )}

        {/* Color Picker for Solid Mode */}
        {isConnected && currentMode === ANIMATION_MODES.MODE_SOLID && (
          <div className="border rounded-lg p-4 space-y-4">
            <h2 className="text-lg font-semibold">Solid Color</h2>
            <div className="flex items-center gap-4">
              <input
                type="color"
                id="colorPicker"
                className="h-12 w-24 cursor-pointer rounded border"
                onChange={(e) => {
                  const hex = e.target.value;
                  const r = parseInt(hex.slice(1, 3), 16);
                  const g = parseInt(hex.slice(3, 5), 16);
                  const b = parseInt(hex.slice(5, 7), 16);
                  setColor(r, g, b).catch(console.error);
                }}
              />
              <label
                htmlFor="colorPicker"
                className="text-sm text-muted-foreground"
              >
                Select color for solid mode
              </label>
            </div>
          </div>
        )}

        {/* Info Section */}
        {!isConnected && (
          <div className="border rounded-lg p-4 bg-muted/50">
            <h3 className="text-sm font-semibold mb-2">How to connect</h3>
            <ul className="text-sm text-muted-foreground space-y-1 list-disc list-inside">
              <li>Make sure your ESP32 device is powered on</li>
              <li>Click the "Connect" button above</li>
              <li>Select "ESP32-LED-Controller" from the device list</li>
              <li>Web Bluetooth requires Chrome, Edge, or Opera browser</li>
            </ul>
          </div>
        )}
      </div>
    </div>
  );
}
