#!/bin/bash
# Escenario: Carga Variable
# 3 procesos fibonacci con diferentes intensidades
# Pregunta: ¿Cuál termina primero? ¿Es justo round-robin?

echo "=== Escenario: Carga Variable ==="
echo "fibonacci con N=10 (rapido), N=25 (medio), N=35 (lento)"
echo ""

cd "$(dirname "$0")/.."

cat <<'EOF' | ./minios
log on
slice 500
run programs/bin/fibonacci
run programs/bin/fibonacci
run programs/bin/fibonacci
EOF
