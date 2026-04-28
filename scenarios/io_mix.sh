#!/bin/bash
# Escenario: IO Mix
# Mezcla de procesos CPU-bound e IO-bound
# Pregunta: ¿Qué proceso tiene más waiting time y por qué?

echo "=== Escenario: IO Mix ==="
echo "primos (CPU-bound) + bitacora (IO-bound) + countdown (mixto)"
echo ""

cd "$(dirname "$0")/.."

cat <<'EOF' | ./minios
log on
slice 500
run programs/bin/primos
run programs/bin/bitacora
run programs/bin/countdown
EOF
