#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

// definicje semaforow
#define SEM_BRIDGE 0
#define SEM_MUTEX 1
#define SEM_ENTRY_GATE 2
#define SEM_LIFO_NOTIFY 3
#define SEM_BRIDGE_EMPTY 4
#define SEM_DISEMBARK 5
#define SEM_COUNT 6

#define MAX_K 100
#define FIFO_NAME "/tmp/tram_log_fifo"

// stany statku
typedef enum {
    LOADING,
    DEPARTING,
    SAILING,
    UNLOADING,
    FINISHED
} ship_state_t;

typedef enum {
    KRAKOW,
    TYNIEC
} port_t;

// struktura pamieci dzielonej
typedef struct {
    int N, M, K, T1, T2, R; // parametry symulacji

    int passengers_on_ship;
    int bikes_on_ship;
    int trip_count;

    ship_state_t ship_state;
    port_t current_port;
    int boarding_closed;

    pid_t captain_pid;

    pid_t bridge_stack[MAX_K]; // stos dla mechanizmu LIFO
    int stack_top;
} shared_state_t;

// zmienne globalne
extern int shm_id;
extern int sem_id;
extern int msg_id;
extern shared_state_t *state;

#endif
