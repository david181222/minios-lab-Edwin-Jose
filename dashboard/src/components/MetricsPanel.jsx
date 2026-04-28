export default function MetricsPanel({ processes, events, currentSlice, connected }) {
  const procs = Object.values(processes);
  const active = procs.filter(p => p.state !== 'TERMINATED').length;
  const terminated = procs.filter(p => p.state === 'TERMINATED').length;
  const totalCpu = procs.reduce((sum, p) => sum + (p.cpuTime || 0), 0);
  const totalSwitches = procs.reduce((sum, p) => sum + (p.switches || 0), 0);
  const switchEvents = events.filter(e => e.type === 'CONTEXT_SWITCH');

  return (
    <div className="space-y-3">
      {/* Connection status */}
      <div className="flex items-center gap-2 text-xs">
        <span className={`w-2 h-2 rounded-full ${connected ? 'bg-green-500' : 'bg-red-500'}`}></span>
        <span className={connected ? 'text-green-400' : 'text-red-400'}>
          {connected ? 'Conectado' : 'Desconectado'}
        </span>
      </div>

      {/* Metrics grid */}
      <div className="grid grid-cols-2 gap-3">
        <MetricCard label="Activos" value={active} color="text-green-400" />
        <MetricCard label="Terminados" value={terminated} color="text-gray-400" />
        <MetricCard label="Time Slice" value={`${currentSlice}ms`} color="text-blue-400" />
        <MetricCard label="Switches" value={totalSwitches} color="text-yellow-400" />
        <MetricCard label="CPU Total" value={`${totalCpu.toFixed(0)}ms`} color="text-purple-400" />
        <MetricCard label="Eventos" value={switchEvents.length} color="text-cyan-400" />
      </div>
    </div>
  );
}

function MetricCard({ label, value, color }) {
  return (
    <div className="bg-gray-800 rounded-lg p-3">
      <div className="text-gray-500 text-xs">{label}</div>
      <div className={`text-lg font-mono font-bold ${color}`}>{value}</div>
    </div>
  );
}
