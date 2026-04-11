import { useState, useEffect, useCallback } from "react";
import { bluetoothService } from "@/services/bluetoothService";
import {
  ANIMATION_MODES,
  TEMPO_TIME_SIGNATURE,
  type AnimationMode,
  type ConnectionState,
  type TempoTimeSignature,
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
  const [heartRateRainbowInPulse, setHeartRateRainbowInPulseState] =
    useState(false);
  const [supportsHeartRateRainbowInPulse, setSupportsHeartRateRainbowInPulse] =
    useState(false);
  const [tempoBpm, setTempoBpmState] = useState(120);
  const [tempoTimeSignature, setTempoTimeSignatureState] =
    useState<TempoTimeSignature>(TEMPO_TIME_SIGNATURE.FOUR_FOUR);
  const [supportsTempoControl, setSupportsTempoControl] = useState(false);

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

      const supportsInPulse =
        bluetoothService.supportsHeartRateRainbowInPulse();
      setSupportsHeartRateRainbowInPulse(supportsInPulse);
      if (supportsInPulse) {
        const inPulse = await bluetoothService.getHeartRateRainbowInPulse();
        setHeartRateRainbowInPulseState(inPulse);
      } else {
        setHeartRateRainbowInPulseState(false);
      }

      const supportsTempo = bluetoothService.supportsTempoControl();
      setSupportsTempoControl(supportsTempo);
      if (supportsTempo) {
        const tempo = await bluetoothService.getTempo();
        setTempoBpmState(tempo.bpm);
        setTempoTimeSignatureState(tempo.timeSignature);
      } else {
        setTempoBpmState(120);
        setTempoTimeSignatureState(TEMPO_TIME_SIGNATURE.FOUR_FOUR);
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
    setSupportsHeartRateRainbowInPulse(false);
    setHeartRateRainbowInPulseState(false);
    setSupportsTempoControl(false);
    setTempoBpmState(120);
    setTempoTimeSignatureState(TEMPO_TIME_SIGNATURE.FOUR_FOUR);
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
      if (enabled && bluetoothService.supportsHeartRateRainbowInPulse()) {
        await bluetoothService.setHeartRateRainbowInPulse(false);
        setHeartRateRainbowInPulseState(false);
      }
    } catch (err) {
      const errorMessage =
        err instanceof Error ? err.message : "Failed to set rainbow option";
      setError(errorMessage);
      throw err;
    }
  }, []);

  const setHeartRateRainbowInPulse = useCallback(async (enabled: boolean) => {
    try {
      setError(null);
      await bluetoothService.setHeartRateRainbowInPulse(enabled);
      setHeartRateRainbowInPulseState(enabled);
      if (enabled && bluetoothService.supportsHeartRateRainbowCycle()) {
        await bluetoothService.setHeartRateRainbowCycle(false);
        setHeartRateRainbowCycleState(false);
      }
    } catch (err) {
      const errorMessage =
        err instanceof Error
          ? err.message
          : "Failed to set rainbow-in-pulse option";
      setError(errorMessage);
      throw err;
    }
  }, []);

  const setTempo = useCallback(
    async (bpm: number, timeSignature: TempoTimeSignature) => {
    try {
      setError(null);
      await bluetoothService.setTempo(bpm, timeSignature);
      setTempoBpmState(Math.max(40, Math.min(240, Math.round(bpm))));
      let sig = timeSignature;
      if (sig < 0 || sig > 3) {
        sig = TEMPO_TIME_SIGNATURE.FOUR_FOUR;
      }
      setTempoTimeSignatureState(sig);
    } catch (err) {
      const errorMessage =
        err instanceof Error ? err.message : "Failed to set tempo";
      setError(errorMessage);
      throw err;
    }
    },
    []
  );

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
    heartRateRainbowInPulse,
    setHeartRateRainbowInPulse,
    supportsHeartRateRainbowInPulse,
    tempoBpm,
    tempoTimeSignature,
    setTempo,
    supportsTempoControl,
    isConnected: connectionState === "connected",
  };
}
