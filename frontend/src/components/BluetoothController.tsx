import { useState, useEffect } from "react";
import { useBluetooth } from "@/hooks/useBluetooth";
import {
  type AnimationMode,
  type TempoTimeSignature,
  ANIMATION_MODES,
  MODES,
  TEMPO_TIME_SIGNATURE,
} from "@/types/bluetooth";
import { Button } from "@/components/ui/button";

const MODE_NAMES: Record<AnimationMode, string> = {
  [ANIMATION_MODES.MODE_OFF]: "Off",
  [ANIMATION_MODES.MODE_SOLID]: "Solid",
  [ANIMATION_MODES.MODE_RAINBOW]: "Rainbow",
  [ANIMATION_MODES.MODE_BREATHING]: "Breathing",
  [ANIMATION_MODES.MODE_CHASE]: "Chase",
  [ANIMATION_MODES.MODE_TWINKLE]: "Twinkle",
  [ANIMATION_MODES.MODE_FIRE]: "Fire",
  [ANIMATION_MODES.MODE_COLOR_WAVE]: "Color Wave",
  [ANIMATION_MODES.MODE_HEART_RATE_PULSE]: "Heart Rate Pulse",
  [ANIMATION_MODES.MODE_TEMPO_PULSE]: "Tempo Pulse",
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
    heartRateRainbowCycle,
    setHeartRateRainbowCycle,
    supportsHeartRateRainbowCycle,
    heartRateRainbowInPulse,
    setHeartRateRainbowInPulse,
    supportsHeartRateRainbowInPulse,
    tempoBpm,
    tempoTimeSignature,
    setTempo,
    supportsTempoControl,
    isConnected,
  } = useBluetooth();

  const pulseStyleMode =
    currentMode === ANIMATION_MODES.MODE_HEART_RATE_PULSE ||
    currentMode === ANIMATION_MODES.MODE_TEMPO_PULSE;

  const [isConnecting, setIsConnecting] = useState(false);
  const [isSettingMode, setIsSettingMode] = useState(false);
  const [tempoBpmDraft, setTempoBpmDraft] = useState(() => String(tempoBpm));

  useEffect(() => {
    setTempoBpmDraft(String(tempoBpm));
  }, [tempoBpm]);

  const commitTempoBpmDraft = () => {
    let v = parseInt(tempoBpmDraft, 10);
    if (Number.isNaN(v)) {
      setTempoBpmDraft(String(tempoBpm));
      return;
    }
    v = Math.max(40, Math.min(240, v));
    setTempoBpmDraft(String(v));
    setTempo(v, tempoTimeSignature).catch(console.error);
  };

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

        {/* Tempo Pulse: BPM and subdivision (firmware BLE) */}
        {isConnected && currentMode === ANIMATION_MODES.MODE_TEMPO_PULSE && (
          <div className="border rounded-lg p-4 space-y-4">
            <h2 className="text-lg font-semibold">Beat &amp; tempo</h2>
            {supportsTempoControl ? (
              <div className="space-y-4">
                <div className="space-y-2">
                  <div className="flex flex-wrap items-baseline justify-between gap-2">
                    <label
                      htmlFor="tempoBpmRange"
                      className="text-sm font-medium"
                    >
                      Tempo (BPM)
                    </label>
                    <span className="text-xs text-muted-foreground">40–240</span>
                  </div>
                  <div className="flex flex-col gap-3 sm:flex-row sm:items-center">
                    <input
                      id="tempoBpmRange"
                      type="range"
                      min={40}
                      max={240}
                      value={tempoBpm}
                      className="w-full sm:flex-1 min-w-0"
                      onChange={(e) => {
                        const v = Number(e.target.value);
                        setTempo(v, tempoTimeSignature).catch(console.error);
                      }}
                    />
                    <div className="flex items-center gap-2 shrink-0">
                      <label htmlFor="tempoBpmNumber" className="sr-only">
                        BPM value
                      </label>
                      <input
                        id="tempoBpmNumber"
                        type="number"
                        min={40}
                        max={240}
                        step={1}
                        inputMode="numeric"
                        className="w-24 border rounded-md px-2 py-1.5 text-sm tabular-nums bg-background"
                        value={tempoBpmDraft}
                        onChange={(e) => setTempoBpmDraft(e.target.value)}
                        onBlur={commitTempoBpmDraft}
                        onKeyDown={(e) => {
                          if (e.key === "Enter") {
                            e.preventDefault();
                            commitTempoBpmDraft();
                            (e.target as HTMLInputElement).blur();
                          }
                        }}
                      />
                      <span className="text-sm text-muted-foreground">BPM</span>
                    </div>
                  </div>
                </div>
                <div className="space-y-2">
                  <label htmlFor="timeSig" className="text-sm font-medium block">
                    Time signature
                  </label>
                  <select
                    id="timeSig"
                    className="w-full border rounded-md px-3 py-2 text-sm bg-background"
                    value={tempoTimeSignature}
                    onChange={(e) => {
                      setTempo(
                        tempoBpm,
                        Number(e.target.value) as TempoTimeSignature
                      ).catch(console.error);
                    }}
                  >
                    <option value={TEMPO_TIME_SIGNATURE.TWO_FOUR}>
                      2/4 (two beats per measure)
                    </option>
                    <option value={TEMPO_TIME_SIGNATURE.THREE_FOUR}>
                      3/4 (waltz)
                    </option>
                    <option value={TEMPO_TIME_SIGNATURE.FOUR_FOUR}>
                      4/4 (common time)
                    </option>
                    <option value={TEMPO_TIME_SIGNATURE.SIX_EIGHT}>
                      6/8 (two beats per bar, compound)
                    </option>
                  </select>
                  <p className="text-xs text-muted-foreground">
                    Bright wave runs once per measure; softer flashes mark each
                    beat in the bar.
                  </p>
                </div>
              </div>
            ) : (
              <p className="text-sm text-muted-foreground">
                Update device firmware to set tempo from the app.
              </p>
            )}
          </div>
        )}

        {/* Color Picker for modes that support color */}
        {isConnected &&
          (currentMode === ANIMATION_MODES.MODE_SOLID ||
            currentMode === ANIMATION_MODES.MODE_BREATHING ||
            currentMode === ANIMATION_MODES.MODE_CHASE ||
            currentMode === ANIMATION_MODES.MODE_TWINKLE ||
            pulseStyleMode) && (
            <div className="border rounded-lg p-4 space-y-4">
              <h2 className="text-lg font-semibold">Color</h2>
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
                  {pulseStyleMode
                    ? "Solid color (wave and dim LEDs, or dim only with rainbow-on-pulse)"
                    : `Select color for ${MODE_NAMES[currentMode].toLowerCase()} mode`}
                </label>
              </div>
              {pulseStyleMode && (
                <div className="pt-3 border-t border-border space-y-3">
                  {supportsHeartRateRainbowCycle ||
                  supportsHeartRateRainbowInPulse ? (
                    <>
                      {supportsHeartRateRainbowCycle && (
                        <label className="flex items-start gap-3 cursor-pointer text-sm">
                          <input
                            type="checkbox"
                            className="mt-1 h-4 w-4 rounded border"
                            checked={heartRateRainbowCycle}
                            onChange={(e) => {
                              setHeartRateRainbowCycle(e.target.checked).catch(
                                console.error
                              );
                            }}
                          />
                          <span>
                            Slowly shift hue for the whole strip (one color at
                            a time)
                          </span>
                        </label>
                      )}
                      {supportsHeartRateRainbowInPulse && (
                        <label className="flex items-start gap-3 cursor-pointer text-sm">
                          <input
                            type="checkbox"
                            className="mt-1 h-4 w-4 rounded border"
                            checked={heartRateRainbowInPulse}
                            onChange={(e) => {
                              setHeartRateRainbowInPulse(
                                e.target.checked
                              ).catch(console.error);
                            }}
                          />
                          <span>
                            Rainbow spectrum on each traveling pulse (scrolls
                            with the wave)
                          </span>
                        </label>
                      )}
                    </>
                  ) : (
                    <p className="text-sm text-muted-foreground">
                      Install the latest firmware on the device to enable
                      rainbow pulse options.
                    </p>
                  )}
                </div>
              )}
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
