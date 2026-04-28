export default function RegisterView({ processes, selectedPid, events }) {
  if (!selectedPid) {
    return (
      <div className="text-gray-500 text-center py-8">
        Selecciona un proceso en la tabla para ver sus detalles.
      </div>
    );
  }

  const proc = processes[selectedPid];
  if (!proc) {
    return (
      <div className="text-gray-500 text-center py-8">
        Proceso {selectedPid} no encontrado.
      </div>
    );
  }

  // Find register events for this PID
  const regEvents = events.filter(e => e.type === 'REGISTERS' && e.pid === selectedPid);
  const lastRegs = regEvents.length > 0 ? regEvents[regEvents.length - 1] : null;

  return (
    <div className="space-y-4">
      <div>
        <h3 className="text-white font-semibold mb-2">
          PID {proc.pid} — {proc.name}
        </h3>
        <div className="grid grid-cols-2 gap-2 text-sm">
          <div className="text-gray-400">Estado:</div>
          <div className="text-white">{proc.state}</div>
          <div className="text-gray-400">CPU Time:</div>
          <div className="text-white font-mono">{(proc.cpuTime || 0).toFixed(1)} ms</div>
          <div className="text-gray-400">Switches:</div>
          <div className="text-white font-mono">{proc.switches || 0}</div>
        </div>
      </div>

      <div>
        <h4 className="text-gray-300 font-semibold mb-2">Registros</h4>
        {lastRegs ? (
          <div className="font-mono text-xs space-y-1">
            <div className="flex justify-between">
              <span className="text-green-400">PC:</span>
              <span className="text-white">{lastRegs.pc}</span>
            </div>
            <div className="flex justify-between">
              <span className="text-blue-400">SP:</span>
              <span className="text-white">{lastRegs.sp}</span>
            </div>
          </div>
        ) : (
          <div className="text-gray-600 text-xs">
            No disponible — SIP habilitado en macOS.
            <br />
            Los registros se mostraran en WSL2/Linux.
          </div>
        )}
      </div>
    </div>
  );
}
