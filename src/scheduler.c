/*
 * scheduler.c — ESQUELETO DEL LABORATORIO
 *
 * Este archivo contiene el núcleo del scheduler round-robin de miniOS.
 * Las funciones de infraestructura (init, getters, install_sigchld, stop,
 * timespec_diff_ms) ya están implementadas.
 *
 * Tu trabajo es implementar las CUATRO funciones marcadas con [TODO]:
 *   1. scheduler_create_process  — fork + exec + SIGSTOP + PCB init
 *   2. scheduler_start           — arrancar el primer proceso y el timer
 *   3. scheduler_tick            — handler de SIGALRM (context switch)
 *   4. scheduler_sigchld         — handler de SIGCHLD (terminación)
 *
 * Cada función viene con comentarios numerados que describen el flujo
 * paso a paso. Tu trabajo es traducir cada paso a código C usando las
 * APIs de POSIX y las funciones de infraestructura disponibles.
 *
 * APIs disponibles:
 *   - POSIX:       fork, execl, waitpid, kill, clock_gettime
 *   - platform_*:  ver src/platform/platform.h
 *   - pcb_*:       ver src/pcb.h
 *   - rq_*:        ver src/ready_queue.h
 *   - timer_*:     ver src/timer.h
 *   - monitor_*:   ver src/monitor.h
 *
 * REGLAS DE SEGURIDAD EN SEÑALES (importantes para scheduler_tick y
 * scheduler_sigchld):
 *   - NO uses printf/fprintf dentro de los handlers (no son
 *     async-signal-safe). Solo kill, waitpid, clock_gettime, write.
 *   - El shell bloquea SIGALRM con sigprocmask durante sus operaciones
 *     críticas, por lo que no necesitas mutex manual sobre process_table.
 */

#include "scheduler.h"
#include "timer.h"
#include "monitor.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <libgen.h>

// Estado global del scheduler
static volatile int current_running = -1;   // índice en process_table del proceso RUNNING, -1 si ninguno
static volatile int scheduler_active = 0;   // 1 si el scheduler está corriendo

// ============================================================
// Helpers ya implementados — NO los modifiques
// ============================================================

double timespec_diff_ms(struct timespec end, struct timespec start) {
    double sec = (double)(end.tv_sec - start.tv_sec);
    double nsec = (double)(end.tv_nsec - start.tv_nsec);
    return sec * 1000.0 + nsec / 1000000.0;
}

void scheduler_init(void) {
    process_count = 0;
    current_running = -1;
    scheduler_active = 0;
    rq_init();
}

int scheduler_get_running(void) {
    return current_running;
}

int scheduler_is_running(void) {
    return scheduler_active;
}

void scheduler_install_sigchld(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = scheduler_sigchld;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);
}

void scheduler_stop(void) {
    timer_stop();
    scheduler_active = 0;

    for (int i = 0; i < process_count; i++) {
        if (process_table[i].state != PROC_TERMINATED) {
            kill(process_table[i].pid, SIGKILL);
            int status;
            waitpid(process_table[i].pid, &status, 0);
            process_table[i].state = PROC_TERMINATED;
        }
    }
    current_running = -1;
}


// ============================================================
// [TODO 1/4] scheduler_create_process
// ------------------------------------------------------------
// Crea un proceso nuevo a partir de un binario, lo deja detenido
// con estado PROC_READY y lo encola en la ready queue.
//
// Retorna el índice del nuevo PCB en process_table, o -1 en error.
//
// Observable correcto: `ps aux | grep <binario>` debe mostrar el
// proceso en estado T (stopped) justo después de crearlo.
// ============================================================
int scheduler_create_process(const char *path, const char *arg) {
    // Paso 1. Validar que hay espacio en process_table.
    if (process_count >= MAX_PROCESSES) {
        fprintf(stderr, "Error: se alcanzó el máximo de procesos (%d).\n", MAX_PROCESSES);
        return -1;
    }

    // Paso 2. fork().
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }

    // Paso 3. Ruta del hijo: habilitar tracing (Linux) y hacer exec.
    if (pid == 0) {
        if (platform_uses_ptrace()) {
            if (platform_trace_child() < 0) {
                perror("platform_trace_child");
                _exit(1);
            }
        }

        if (arg != NULL) {
            execl(path, path, arg, NULL);
        } else {
            execl(path, path, NULL);
        }

        // Si llegamos aquí, execl falló.
        perror("execl");
        _exit(1);
    }

    int status = 0;

    // Paso 4. En Linux con ptrace: esperar el SIGTRAP post-exec.
    if (platform_uses_ptrace()) {
        if (waitpid(pid, &status, 0) < 0) {
            perror("waitpid");
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            return -1;
        }
        if (!WIFSTOPPED(status)) {
            fprintf(stderr, "Error: el hijo no quedó detenido tras exec (status=%d).\n", status);
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            return -1;
        }
    }

    // Paso 5. Inicializar PCB con un nombre corto (basename) para la UI.
    int idx = process_count;
    const char *pcb_name = path;
    char *path_copy = strdup(path);
    if (path_copy != NULL) {
        pcb_name = basename(path_copy);
    }
    pcb_init(&process_table[idx], pid, pcb_name);
    free(path_copy);

    // Paso 6. (Linux/ptrace) Capturar registros iniciales si es posible.
    if (platform_uses_ptrace()) {
        if (platform_get_registers(pid, &process_table[idx].registers) == 0) {
            process_table[idx].regs_valid = 1;
        }
        // Importante: dejamos el proceso detenido con SIGSTOP antes de soltar ptrace.
        if (platform_stop_process(pid) < 0) {
            perror("platform_stop_process");
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            return -1;
        }
        if (platform_detach(pid) < 0) {
            perror("platform_detach");
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            return -1;
        }
    } else {
        // En plataformas sin ptrace, al menos detenemos el proceso explícitamente.
        if (platform_stop_process(pid) < 0) {
            perror("platform_stop_process");
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            return -1;
        }
    }

    // Paso 8. Confirmar que el proceso quedó detenido (estado 'T').
    if (waitpid(pid, &status, WUNTRACED) < 0) {
        perror("waitpid");
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
        return -1;
    }

    // Paso 9. Marcar READY, encolar y emitir evento de creación.
    process_table[idx].state = PROC_READY;

    if (rq_enqueue(idx) < 0) {
        fprintf(stderr, "Error: no se pudo encolar el proceso en la ready queue.\n");
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
        process_table[idx].state = PROC_TERMINATED;
        return -1;
    }

    process_count++;
    monitor_emit_created(process_table[idx].pid, process_table[idx].name);
    return idx;
}


// ============================================================
// [TODO 2/4] scheduler_start
// ------------------------------------------------------------
// Arranca el scheduler: desencola el primer proceso, lo pone en
// RUNNING, le manda SIGCONT e instala el timer que dispara el
// context switch periódicamente.
//
// Observable correcto: tras crear 1 proceso y llamar start, ese
// proceso empieza a producir salida en la terminal.
// ============================================================
void scheduler_start(int slice_ms) {
    // Paso 1. Si rq_is_empty(), imprimir "No hay procesos en la ready queue."
    //         y retornar.

    // Paso 2. Desencolar el primer índice con rq_dequeue().

    // Paso 3. Actualizar PCB del proceso entrante:
    //         - process_table[idx].state = PROC_RUNNING;
    //         - clock_gettime(CLOCK_MONOTONIC, &process_table[idx].last_started);
    //         - current_running = idx;

    // Paso 4. Reanudar el proceso con platform_resume_process(process_table[idx].pid).

    // Paso 5. Activar el scheduler y arrancar el timer:
    //         - scheduler_active = 1;
    //         - timer_init(slice_ms, scheduler_tick);  // registra el handler
    //         - timer_start();                         // arranca setitimer

    // Paso 1. Si la cola está vacía, no hay nada que ejecutar.
    if (rq_is_empty()) {
        printf("No hay procesos en la ready queue.\n");
        return;
    }

    // Paso 2. Desencolar el primer proceso.
    int idx = rq_dequeue();
    if (idx < 0) {
        printf("No hay procesos en la ready queue.\n");
        return;
    }

    // Paso 3. Actualizar PCB entrante y registrar timestamp.
    process_table[idx].state = PROC_RUNNING;
    clock_gettime(CLOCK_MONOTONIC, &process_table[idx].last_started);
    current_running = idx;

    // Paso 4. Reanudar el proceso.
    if (platform_resume_process(process_table[idx].pid) < 0) {
        perror("platform_resume_process");
        process_table[idx].state = PROC_READY;
        current_running = -1;
        rq_enqueue(idx);
        return;
    }

    // Paso 5. Activar el scheduler y arrancar el timer periódico.
    scheduler_active = 1;
    timer_init(slice_ms, scheduler_tick);
    timer_start();
}


// ============================================================
// [TODO 3/4] scheduler_tick  — handler de SIGALRM
// ------------------------------------------------------------
// Se invoca CADA vez que expira el time slice. Realiza el
// context switch: detiene al proceso actual, actualiza su PCB,
// lo manda al final de la cola, saca al siguiente y lo reanuda.
//
// ¡IMPORTANTE! Esta función corre en un signal handler.
//   - NO llames printf/fprintf/malloc.
//   - Solo kill, waitpid, clock_gettime, write son seguros.
//   - Las funciones monitor_emit_* internamente usan snprintf + write,
//     que es aceptable para este proyecto educativo.
//
// Observable correcto: el Gantt chart del dashboard muestra
// segmentos alternados entre procesos cada ~slice_ms.
// ============================================================
void scheduler_tick(int signum) {
    (void)signum;

    // Paso 1. Salida temprana: si current_running < 0 o !scheduler_active, return.

    // Paso 2. Obtener puntero al PCB del proceso actual:
    //         pcb_t *current = &process_table[current_running];

    // Paso 3. Detener el proceso actual con platform_stop_process(current->pid).

    // Paso 4. Actualizar PCB del saliente:
    //         - Obtener tiempo actual con clock_gettime(CLOCK_MONOTONIC, &now).
    //         - Calcular elapsed = timespec_diff_ms(now, current->last_started).
    //         - current->cpu_time_ms += elapsed;
    //         - current->state = PROC_READY;
    //         - current->context_switches++;

    // Paso 5. Encolar el proceso saliente con rq_enqueue(current_running).

    // Paso 6. Si la cola quedó vacía (rq_is_empty()):
    //         - current_running = -1;
    //         - timer_stop();
    //         - return;

    // Paso 7. Desencolar el siguiente, actualizar su PCB y reanudarlo:
    //         - int next_idx = rq_dequeue();
    //         - pcb_t *next = &process_table[next_idx];
    //         - next->state = PROC_RUNNING;
    //         - clock_gettime(CLOCK_MONOTONIC, &next->last_started);
    //         - platform_resume_process(next->pid);
    //         - monitor_emit_switch(current->pid, next->pid, timer_get_slice());
    //         - current_running = next_idx;
}


// ============================================================
// [TODO 4/4] scheduler_sigchld  — handler de SIGCHLD
// ------------------------------------------------------------
// Se invoca cuando un proceso hijo termina. Debe:
//   - Detectar TODOS los hijos terminados (puede haber varios).
//   - Actualizar su PCB a PROC_TERMINATED.
//   - Si el proceso que terminó era el RUNNING, despachar al siguiente.
//   - Si estaba en la ready queue, removerlo con rq_remove().
//
// Observable correcto: tras `exit` en miniOS, `ps aux | grep defunct`
// no muestra zombies.
// ============================================================
void scheduler_sigchld(int signum) {
    (void)signum;

    // Paso 1. Loop while waitpid(-1, &status, WNOHANG | WUNTRACED) > 0:
    //         - WNOHANG para no bloquear.
    //         - El loop recoge todos los hijos terminados pendientes.

    // Paso 2. Dentro del loop, IGNORAR paradas: si !WIFEXITED(status) y
    //         !WIFSIGNALED(status), continue (el proceso solo se detuvo,
    //         no terminó).

    // Paso 3. Buscar el PID en process_table (loop por los process_count).
    //         Ignorar si ya está PROC_TERMINATED.

    // Paso 4. Marcar process_table[i].state = PROC_TERMINATED y
    //         llamar monitor_emit_terminated(pid, cpu_time_ms, context_switches).

    // Paso 5. Si i == current_running (el que terminó era el que corría):
    //         a) Actualizar cpu_time_ms con el elapsed desde last_started.
    //         b) current_running = -1;
    //         c) Si !rq_is_empty(): desencolar siguiente, marcarlo RUNNING,
    //            registrar last_started y platform_resume_process.
    //            Luego current_running = next.
    //         d) Si rq_is_empty(): timer_stop(); scheduler_active = 0;

    // Paso 6. Si i != current_running (estaba en la cola esperando):
    //         rq_remove(i);
}
