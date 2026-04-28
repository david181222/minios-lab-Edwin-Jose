import { useEffect, useState } from 'react';

export default function CaptureControls({
  isCapturing,
  onStart,
  onStop,
  onReset,
  getRelativeTime,
}) {
  const [elapsed, setElapsed] = useState(0);

  useEffect(() => {
    const interval = setInterval(() => {
      setElapsed(getRelativeTime());
    }, 100);
    return () => clearInterval(interval);
  }, [getRelativeTime]);

  const formatDuration = (s) => {
    const m = Math.floor(s / 60);
    const sec = Math.floor(s % 60);
    const ms = Math.floor((s - Math.floor(s)) * 10);
    return `${String(m).padStart(2, '0')}:${String(sec).padStart(2, '0')}.${ms}`;
  };

  return (
    <div className="flex items-center gap-3 bg-gray-900 rounded-xl border border-gray-800 px-4 py-3">
      {/* Record / Stop button */}
      {isCapturing ? (
        <button
          onClick={onStop}
          className="flex items-center gap-2 px-3 py-1.5 bg-red-600 hover:bg-red-500 text-white rounded-lg font-semibold text-sm transition-colors"
          title="Detener captura"
        >
          <span className="w-3 h-3 bg-white rounded-sm"></span>
          Detener
        </button>
      ) : (
        <button
          onClick={onStart}
          className="flex items-center gap-2 px-3 py-1.5 bg-gray-800 hover:bg-gray-700 text-red-400 border border-red-900 rounded-lg font-semibold text-sm transition-colors"
          title="Iniciar captura"
        >
          <span className="w-3 h-3 bg-red-500 rounded-full animate-pulse"></span>
          Capturar
        </button>
      )}

      {/* Reset button */}
      <button
        onClick={onReset}
        className="px-3 py-1.5 bg-gray-800 hover:bg-gray-700 text-gray-300 rounded-lg text-sm transition-colors"
        title="Limpiar eventos capturados"
      >
        Limpiar
      </button>

      {/* Status indicator */}
      <div className="flex items-center gap-2 ml-2">
        {isCapturing ? (
          <>
            <span className="w-2 h-2 bg-red-500 rounded-full animate-pulse"></span>
            <span className="text-red-400 text-xs font-semibold">CAPTURANDO</span>
          </>
        ) : (
          <>
            <span className="w-2 h-2 bg-gray-500 rounded-full"></span>
            <span className="text-gray-500 text-xs font-semibold">PAUSADO</span>
          </>
        )}
      </div>

      {/* Elapsed time */}
      <div className="ml-auto flex items-center gap-2">
        <span className="text-gray-500 text-xs">Tiempo:</span>
        <span className="text-gray-200 font-mono text-sm tabular-nums">
          {formatDuration(elapsed)}
        </span>
      </div>
    </div>
  );
}
