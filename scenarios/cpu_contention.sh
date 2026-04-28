#!/bin/bash
# Escenario: CPU Contention
# 3 procesos CPU-bound compitiendo por el CPU
# Pregunta: ¿Cómo afecta el time slice al turnaround time?

echo "=== Escenario: CPU Contention ==="
echo "3 procesos primos con rangos diferentes"
echo "Prueba con slice 500 y luego con slice 200"
echo ""

cd "$(dirname "$0")/.."

cat <<'EOF' | ./minios
log on
slice 500
run programs/bin/primos
run programs/bin/primos
run programs/bin/primos
EOF
