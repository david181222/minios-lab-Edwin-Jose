# miniOS — Simulador Educativo de Context Switching

## 1. Visión del Proyecto

Prototipo educativo que demuestra **context switching y scheduling** de procesos usando mecanismos reales del sistema operativo. Los estudiantes interactúan con un shell propio (`miniOS>`) para lanzar procesos reales compilados en C, mientras un scheduler round-robin los administra con un time slice configurable.

**Valor pedagógico central:** El estudiante ve cómo el PCB guarda el estado (incluyendo registros reales del CPU), cómo se restaura, y cómo el time slice determina el comportamiento del sistema.

---

## 2. Arquitectura General

```
┌─────────────────────────────────────────────────────────┐
│                    CAPA DE VISUALIZACIÓN                 │
│  ┌─────────────┐    WebSocket    ┌───────────────────┐  │
│  │  Bridge      │◄──────────────►│  Dashboard React   │  │
│  │  (Node.js)   │                │  (localhost:3000)  │  │
│  └──────▲───────┘                └───────────────────┘  │
│         │ Named Pipe / Unix Socket                      │
├─────────┼───────────────────────────────────────────────┤
│         │          CAPA DE SCHEDULING (C)                │
│  ┌──────┴───────────────────────────────────────────┐   │
│  │              SCHEDULER (main loop)                │   │
│  │  ┌──────────┐  ┌───────────┐  ┌──────────────┐  │   │
│  │  │  Shell    │  │  Timer    │  │  Event       │  │   │
│  │  │ miniOS>  │  │ SIGALRM   │  │  Logger      │  │   │
│  │  └──────────┘  └───────────┘  └──────────────┘  │   │
│  │  ┌──────────────────────────────────────────┐    │   │
│  │  │         Process Table (PCBs)              │    │   │
│  │  │  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐       │    │   │
│  │  │  │PCB 0│ │PCB 1│ │PCB 2│ │PCB N│       │    │   │
│  │  │  └─────┘ └─────┘ └─────┘ └─────┘       │    │   │
│  │  └──────────────────────────────────────────┘    │   │
│  │  ┌──────────────────────────────────────────┐    │   │
│  │  │         Ready Queue (Round-Robin)         │    │   │
│  │  └──────────────────────────────────────────┘    │   │
│  └──────────────────────────────────────────────────┘   │
│         │ fork() + exec() + ptrace + SIGSTOP/SIGCONT    │
├─────────┼───────────────────────────────────────────────┤
│         ▼       PROCESOS HIJO (binarios C)              │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐              │
│  │ primos   │  │ fibonacci│  │ ping-pong│  ...          │
│  └──────────┘  └──────────┘  └──────────┘              │
├─────────────────────────────────────────────────────────┤
│                KERNEL DE LINUX / macOS                   │
│   (context switch real, señales, ptrace, sockets)       │
└─────────────────────────────────────────────────────────┘
```

---

## 3. Componentes del Scheduler (C)

### 3.1 PCB (Process Control Block)

```c
// Estados del proceso
typedef enum {
    PROC_NEW,        // Recién creado con fork, aún no ha corrido
    PROC_READY,      // En la ready queue, esperando CPU
    PROC_RUNNING,    // Actualmente ejecutándose
    PROC_BLOCKED,    // Bloqueado en IO (socket, pipe, disco)
    PROC_TERMINATED  // Finalizado
} proc_state_t;

// Registros normalizados (abstracción cross-platform)
typedef struct {
    uint64_t program_counter;  // RIP en x86_64, PC en ARM64
    uint64_t stack_pointer;    // RSP en x86_64, SP en ARM64
    uint64_t general_regs[16]; // Registros de propósito general
} cpu_context_t;

// PCB
typedef struct {
    int               pid;              // PID real del proceso hijo
    char              name[64];         // Nombre/ruta del binario
    proc_state_t      state;            // Estado lógico actual
    cpu_context_t     registers;        // Último snapshot de registros (ptrace)
    struct timespec   created_at;       // Timestamp de creación
    struct timespec   last_started;     // Última vez que entró a RUNNING
    double            cpu_time_ms;      // Tiempo acumulado de CPU (ms)
    double            wait_time_ms;     // Tiempo acumulado en espera (ms)
    int               context_switches; // Número de context switches
} pcb_t;
```

### 3.2 Process Table y Ready Queue

```c
#define MAX_PROCESSES 10

// Tabla de procesos (arreglo estático)
pcb_t process_table[MAX_PROCESSES];
int   process_count = 0;

// Ready queue (arreglo circular de índices a la process table)
int ready_queue[MAX_PROCESSES];
int queue_front = 0;
int queue_rear = 0;
int queue_size = 0;
```

### 3.3 Flujo del Context Switch

```
SIGALRM dispara (time slice expirado)
  │
  ├─► SIGSTOP al proceso RUNNING actual
  ├─► ptrace(PTRACE_GETREGS) → captura registros → guarda en PCB
  ├─► Actualizar PCB: estado = READY, acumular cpu_time
  ├─► Encolar proceso al final de ready_queue
  ├─► Emitir evento CONTEXT_SWITCH al logger
  │
  ├─► Desencolar siguiente proceso de ready_queue
  ├─► Actualizar PCB: estado = RUNNING, registrar timestamp
  ├─► ptrace(PTRACE_GETREGS) → captura registros post-restore
  ├─► SIGCONT al nuevo proceso
  │
  └─► Reiniciar timer para siguiente slice
```

### 3.4 Creación de Procesos

```
Usuario escribe: run ./mi_programa
  │
  ├─► fork()
  │     └─► Hijo: ptrace(PTRACE_TRACEME) → execvp("./mi_programa")
  │                 └─► Kernel detiene al hijo con SIGTRAP (post-exec)
  │
  ├─► Padre: waitpid() hasta SIGTRAP
  ├─► Capturar registros iniciales con ptrace
  ├─► SIGSTOP al hijo
  ├─► Crear PCB: estado = READY
  ├─► Encolar en ready_queue
  └─► Emitir evento PROCESS_CREATED
```

---

## 4. Shell Interactivo

### Comandos esenciales

| Comando           | Descripción                                    |
|--------------------|------------------------------------------------|
| `run <binario>`    | Lanza un proceso nuevo (fork + exec)           |
| `ps`               | Muestra la process table con PCBs              |
| `kill <pid>`       | Termina un proceso                             |
| `slice <ms>`       | Cambia el time slice en caliente               |
| `exit`             | Termina todos los procesos y sale              |

### Comandos de monitoreo

| Comando            | Descripción                                    |
|--------------------|------------------------------------------------|
| `inspect <pid>`    | Muestra registros capturados del proceso       |
| `log on/off`       | Activa/desactiva log de context switches       |
| `stats`            | Muestra métricas agregadas                     |

### Concurrencia Shell ↔ Scheduler

- **Main thread:** maneja el prompt, parsea comandos, lee/escribe la process table.
- **SIGALRM handler:** ejecuta la lógica de context switch de forma asíncrona.
- **Sincronización:** mutex sobre la process table para evitar lecturas durante un switch.

---

## 5. Capa de Abstracción de Plataforma

```
src/
├── platform/
│   ├── platform.h          # Interfaz común
│   ├── platform_linux.c    # Implementación Linux (ptrace + user_regs_struct)
│   └── platform_darwin.c   # Implementación macOS (task_threads + thread_get_state)
```

### Interfaz

```c
// platform.h
int   platform_trace_child(pid_t pid);          // Configurar tracing
int   platform_stop_process(pid_t pid);         // Detener proceso
int   platform_resume_process(pid_t pid);       // Reanudar proceso
int   platform_get_registers(pid_t pid, cpu_context_t *ctx);  // Capturar registros
```

### Diferencias clave

| Aspecto             | Linux (WSL2)              | macOS (Apple Silicon)        |
|---------------------|---------------------------|------------------------------|
| Arquitectura        | x86_64                    | ARM64                        |
| Captura registros   | `ptrace(PTRACE_GETREGS)`  | `thread_get_state()` (Mach)  |
| Struct de registros | `user_regs_struct`        | `arm_thread_state64_t`       |
| Program Counter     | `regs.rip`                | `state.__pc`                 |
| Stack Pointer       | `regs.rsp`                | `state.__sp`                 |

---

## 6. Capa de Observabilidad

### 6.1 Eventos (JSON por línea)

```json
{"type":"CONTEXT_SWITCH","timestamp":1234567890.123,"from_pid":1001,"to_pid":1002,"from_state":"READY","to_state":"RUNNING","slice_ms":500}
{"type":"PROCESS_CREATED","timestamp":1234567890.456,"pid":1003,"name":"./primos","registers":{"pc":"0x401234","sp":"0x7ffd1234"}}
{"type":"PROCESS_TERMINATED","timestamp":1234567891.789,"pid":1001,"cpu_time_ms":2340.5,"context_switches":12}
{"type":"REGISTERS_SNAPSHOT","timestamp":1234567890.600,"pid":1002,"pc":"0x401580","sp":"0x7ffd5678","regs":["0x1","0x5","0x0"]}
```

### 6.2 Pipeline de datos

```
Scheduler (C) ──► Named Pipe ──► Bridge (Node.js) ──► WebSocket ──► Dashboard (React)
                  /tmp/minios.pipe      ~30 líneas
```

### 6.3 Dashboard Web (React + Vite + Tailwind)

**Panel 1 — Gantt Chart:** Cada proceso es una fila con barra de color que crece en RUNNING. Los context switches son los cortes entre colores.

**Panel 2 — Process Table:** PCBs como tarjetas con PID, estado (badge de color), CPU time, wait time, switches. Transiciones animadas.

**Panel 3 — Registros:** Al seleccionar un proceso, muestra PC, SP, registros generales. Highlight de valores que cambiaron desde la última captura.

**Panel 4 — Métricas:** CPU utilization, throughput, average waiting time, time slice actual.

---

## 7. Procesos de Ejemplo

### Independientes (CPU-bound / IO-bound)

| Proceso          | Tipo      | Descripción                                                   | Parámetro          |
|------------------|-----------|---------------------------------------------------------------|---------------------|
| `primos`         | CPU-bound | Busca primos secuencialmente, imprime cada hallazgo           | Rango máximo        |
| `fibonacci`      | CPU-bound | Fibonacci recursivo, stack crece visiblemente                 | Número a calcular   |
| `countdown`      | Mixto     | Cuenta regresiva imprimiendo cada segundo                     | Número inicial      |
| `artista_ascii`  | Mixto     | Dibuja un patrón ASCII línea por línea                        | Tamaño del patrón   |
| `loteria`        | CPU-bound | Genera aleatorios buscando un patrón (3 iguales consecutivos) | Semilla             |
| `bitacora`       | IO-bound  | Escribe líneas con timestamp a un archivo                     | Intervalo (ms)      |
| `memory_hog`     | Memoria   | Pide memoria incrementalmente con malloc                      | Tamaño total (MB)   |

### Comunicantes (AF_UNIX sockets)

| Proceso              | Descripción                                                             |
|----------------------|-------------------------------------------------------------------------|
| `ping_pong`          | Dos procesos pasan un contador por socket, ida y vuelta                 |
| `productor_consumidor` | Uno genera datos, otro los consume. Demuestra bloqueo por buffer lleno/vacío |
| `chat`               | N clientes + 1 servidor central. Mensajes broadcast                     |
| `task_dispatcher`    | Dispatcher envía tareas a workers por socket. Patrón thread-pool con procesos |

### Escenarios predefinidos

| Escenario            | Procesos                                    | Pregunta de laboratorio                          |
|----------------------|---------------------------------------------|--------------------------------------------------|
| CPU Contention       | 3× primos (rangos altos)                    | ¿Cómo afecta el time slice al turnaround time?   |
| IO Mix               | 1× primos + 1× bitácora + 1× countdown     | ¿Qué proceso tiene más waiting time y por qué?   |
| Comunicación         | ping_pong + 1× fibonacci                    | ¿Cuándo entra un proceso en BLOCKED?             |
| Carga variable       | 3× fibonacci (N=10, N=30, N=45)             | ¿Cuál termina primero? ¿Es justo round-robin?    |

---

## 8. Estructura del Repositorio

```
final_SO/
├── ARCHITECTURE.md              # Este documento
├── Makefile                     # Build cross-platform
│
├── src/                         # Scheduler (C)
│   ├── main.c                   # Entry point, shell loop
│   ├── scheduler.c/h            # Lógica de scheduling y context switch
│   ├── pcb.c/h                  # Estructuras PCB y process table
│   ├── ready_queue.c/h          # Cola circular
│   ├── shell.c/h                # Parser de comandos del shell
│   ├── monitor.c/h              # Logger de eventos (JSON)
│   ├── timer.c/h                # Configuración de setitimer / SIGALRM
│   └── platform/
│       ├── platform.h           # Interfaz abstracta
│       ├── platform_linux.c     # ptrace Linux x86_64
│       └── platform_darwin.c    # Mach API macOS ARM64
│
├── programs/                    # Procesos de ejemplo (C)
│   ├── primos.c
│   ├── fibonacci.c
│   ├── countdown.c
│   ├── artista_ascii.c
│   ├── loteria.c
│   ├── bitacora.c
│   ├── memory_hog.c
│   ├── ping_pong_server.c
│   ├── ping_pong_client.c
│   ├── productor.c
│   ├── consumidor.c
│   └── task_dispatcher.c
│
├── scenarios/                   # Escenarios de laboratorio
│   ├── cpu_contention.conf
│   ├── io_mix.conf
│   ├── comunicacion.conf
│   └── carga_variable.conf
│
├── bridge/                      # Bridge Node.js (WebSocket)
│   ├── package.json
│   └── index.js
│
└── dashboard/                   # Visualización (React + Vite)
    ├── package.json
    ├── vite.config.js
    └── src/
        ├── App.jsx
        ├── components/
        │   ├── GanttChart.jsx
        │   ├── ProcessTable.jsx
        │   ├── RegisterView.jsx
        │   └── MetricsPanel.jsx
        └── hooks/
            └── useSchedulerEvents.js
```

---

## 9. División de Responsabilidades: Scheduler vs Kernel

| Responsabilidad               | Quién lo hace     | Mecanismo                    |
|-------------------------------|-------------------|------------------------------|
| Decidir quién corre           | **Scheduler (tú)**| Ready queue + round-robin    |
| Definir el time slice         | **Scheduler (tú)**| setitimer + SIGALRM          |
| Mantener metadata de procesos | **Scheduler (tú)**| PCB en process table         |
| Salvar registros del CPU      | **Kernel**         | SIGSTOP + ptrace internals   |
| Restaurar registros del CPU   | **Kernel**         | SIGCONT                      |
| Swap de page tables           | **Kernel**         | Automático al cambiar proceso|
| Crear procesos                | **Ambos**          | fork (kernel) + exec (kernel) + PCB init (tú) |
| Inspeccionar registros        | **Ambos**          | ptrace (kernel provee) + tú los lees y almacenas |

> **Lección clave:** El scheduler es *política* (quién, cuándo, cuánto). El kernel es *mecanismo* (cómo se congela/descongela un proceso). Esta separación es un principio fundamental de diseño de OS.

---

## 10. Métricas Calculables

| Métrica            | Fórmula                                              | Qué revela                               |
|--------------------|------------------------------------------------------|------------------------------------------|
| Turnaround Time    | `terminated_at - created_at`                         | Tiempo total de vida del proceso          |
| Waiting Time       | `turnaround_time - cpu_time`                         | Tiempo que pasó sin ejecutarse            |
| CPU Utilization    | `sum(cpu_time) / wall_clock_time × 100`              | % del tiempo que el CPU estuvo ocupado    |
| Throughput         | `procesos_terminados / wall_clock_time`              | Procesos completados por unidad de tiempo |
| Context Switches   | Contador por proceso y total                         | Overhead del scheduling                   |
| Avg Response Time  | `promedio(primer_run - created_at)`                  | Latencia para que un proceso empiece      |
