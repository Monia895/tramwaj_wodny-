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

#define SEM_BRIDGE 0
#define SEM_STATE 1
#define SEM_SHIP 2
#define SEM_LOG 3
#define SEM_COUNT 4

#define TRAM_FIFO "/tmp/tram_fifo"

extern int N;
extern int M;
extern int K;
extern int T1;
extern int T2;
extern int R;

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

typedef enum {
TO_SHIP,
FROM_SHIP
} bridge_dir_t;


typedef struct {
int passengers_on_ship;
int bikes_on_ship;
int passengers_on_bridge;
int trip_count;
pid_t captain_pid;
ship_state_t ship_state;
port_t current_port;
int boarding_closed;
bridge_dir_t bridge_dir;
} shared_state_t;

static inline void sem_lock(int sem_id, int sem_num) {
    struct sembuf op = {sem_num, -1, 0};
    semop(sem_id, &op, 1);
}

static inline void sem_unlock(int sem_id, int sem_num) {
    struct sembuf op = {sem_num, 1, 0};
    semop(sem_id, &op, 1);
}

extern int shm_id;
extern int sem_id;
extern shared_state_t *state;
extern int msg_id;

#endif
