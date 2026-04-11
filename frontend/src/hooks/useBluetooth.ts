import { useState, useEffect, useCallback } from "react";
import { bluetoothService } from "@/services/bluetoothService";
import {
  ANIMATION_MODES,
  type AnimationMode,
  type ConnectionState,
} from "@/types/bluetooth";

export function useBluetooth() {
  const [connectionState, setConnectionState] =
    useState<ConnectionState>("disconnected");
  const [currentMode, setCurrentMode] = useState<AnimationMode>(
    ANIMATION_MODES.MODE_OFF
  );
  const [error, setError] = useState<string | null>(null);
  const [heartRateRainbowCycle, setHeartRateRainbowCycleState] =
    useState(false);
  const [supportsHeartRateRainbowCycle, setSupportsHeartRateRainbowCycle] =
    useState(false);

  useEffect(() => {
    // Subscribe to connection state changes
    const handleConnectionStateChange = (state: ConnectionState) => {
      setConnectionState(state);
      if (state === "error") {
        setError("Connection error occurred");
      } else {
        setError(null);
      }
    };

    // Subscribe to mode changes
    const handleModeChange = (mode: AnimationMode) => {
      setCurrentMode(mode);
    };

    bluetoothService.onConnectionStateChange(handleConnectionStateChange);
    bluetoothService.onModeChange(handleModeChange);

    // Read initial connection state
    const initialConnectionState = bluetoothService.getConnectionState();
    setTimeout(() => {
      setConnectionState(initialConnectionState);
    }, 0);

    return () => {
      // Cleanup is handled by the service singleton
    };
  }, []);

  const connect = useCallback(async () => {
    try {
      setError(null);
      await bluetoothService.connect();
      // Read current mode after connection
      const mode = await bluetoothService.getCurrentMode();
      setCurrentMode(mode);

      const supportsRainbow =
        bluetoothService.supportsHeartRateRainbowCycle();
      setSupportsHeartRateRainbowCycle(supportsRainbow);
      if (supportsRainbow) {
        const rainbow = await bluetoothService.getHeartRateRainbowCycle();
        setHeartRateRainbowCycleState(rainbow);
      } else {
        setHeartRateRainbowCycleState(false);
      }
    } catch (err) {
      const errorMessage =
        err instanceof Error ? err.message : "Failed to connect";
      setError(errorMessage);
      throw err;
    }
  }, []);

  const disconnect = useCallback(() => {
    bluetoothService.disconnect();
    setError(null);
    setSupportsHeartRateRainbowCycle(false);
    setHeartRateRainbowCycleState(false);
  }, []);

  const setMode = useCallback(async (mode: AnimationMode) => {
    try {
      setError(null);
      await bluetoothService.setMode(mode);
    } catch (err) {
      const errorMessage =
        err instanceof Error ? err.message : "Failed to set mode";
      setError(errorMessage);
      throw err;
    }
  }, []);

  const setHeartRateRainbowCycle = useCallback(async (enabled: boolean) => {
    try {
      setError(null);
      await bluetoothService.setHeartRateRainbowCycle(enabled);
      setHeartRateRainbowCycleState(enabled);
    } catch (err) {
      const errorMessage =
        err instanceof Error ? err.message : "Failed to set rainbow option";
      setError(errorMessage);
      throw err;
    }
  }, []);

  const setColor = useCallback(async (r: number, g: number, b: number) => {
    try {
      setError(null);
      await bluetoothService.setColor(r, g, b);
    } catch (err) {
      const errorMessage =
        err instanceof Error ? err.message : "Failed to set color";
      setError(errorMessage);
      throw err;
    }
  }, []);

  return {
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
    isConnected: connectionState === "connected",
  };
}
