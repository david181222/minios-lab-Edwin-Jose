#include "ready_queue.h"
#include <stdio.h>

static int queue[MAX_PROCESSES];
static int front = 0;
static int rear = 0;
static int count = 0;

void rq_init(void) {
    front = 0;
    rear = 0;
    count = 0;
}

int rq_enqueue(int idx) {
    if (count >= MAX_PROCESSES) return -1;
    queue[rear] = idx;
    rear = (rear + 1) % MAX_PROCESSES;
    count++;
    return 0;
}

int rq_dequeue(void) {
    if (count == 0) return -1;
    int idx = queue[front];
    front = (front + 1) % MAX_PROCESSES;
    count--;
    return idx;
}

int rq_peek(void) {
    if (count == 0) return -1;
    return queue[front];
}

int rq_size(void) {
    return count;
}

int rq_is_empty(void) {
    return count == 0;
}

int rq_remove(int idx) {
    // Rebuild queue without the target index
    int new_queue[MAX_PROCESSES];
    int new_count = 0;
    int found = 0;

    for (int i = 0; i < count; i++) {
        int pos = (front + i) % MAX_PROCESSES;
        if (queue[pos] == idx) {
            found = 1;
        } else {
            new_queue[new_count++] = queue[pos];
        }
    }

    if (!found) return -1;

    // Copy back
    for (int i = 0; i < new_count; i++) {
        queue[i] = new_queue[i];
    }
    front = 0;
    rear = new_count;
    count = new_count;
    return 0;
}

void rq_print(void) {
    if (count == 0) {
        printf("  Ready Queue: [vacía]\n");
        return;
    }
    printf("  Ready Queue: ");
    for (int i = 0; i < count; i++) {
        int pos = (front + i) % MAX_PROCESSES;
        int idx = queue[pos];
        printf("PID %d", process_table[idx].pid);
        if (i < count - 1) printf(" → ");
    }
    printf("\n");
}
