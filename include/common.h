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

extern int N;
extern int M;
extern int K;
extern int T1;
extern int T2;
extern int R;

extern int shm_id;
extern int sem_id;
extern shared_state_t *state;

typedef enum {
LOADING,
SAILING,
UNLOADING,
STOPPED
} ship_state_t;

typedef struct {
int passengers_on_ship;
int bikes_on_ship;
int passengers_on_bridge;
} shared_state_t;

#endif
