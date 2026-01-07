#define _POSIX_C_SOURCE 200809L
#include "ipc.h"
#include "msg.h"
#include <sys/msg.h>

// zmienne globalne
int shm_id = -1;
int sem_id = -1;
int msg_id = -1;
shared_state_t *state = NULL;

void ipc_init_all(int N, int M, int K, int T1, int T2, int R) {
    key_t key = ftok("Makefile", 'A');
    if (key == -1) { perror("ftok"); exit(1); }

    shm_id = shmget(key, sizeof(shared_state_t), IPC_CREAT | 0600);
    if (shm_id == -1) { perror("shmget"); exit(1); }

    state = shmat(shm_id, NULL, 0);
    if (state == (void*)-1) { perror("shmat"); exit(1); }

    // inicjalizacja wartosci
    state->N = N; state->M = M; state->K = K;
    state->T1 = T1; state->T2 = T2; state->R = R;
    state->passengers_on_ship = 0;
    state->bikes_on_ship = 0;
    state->trip_count = 0;
    state->ship_state = LOADING;
    state->current_port = KRAKOW;
    state->boarding_closed = 0;
    state->stack_top = 0;

    // semafory
    sem_id = semget(key, SEM_COUNT, IPC_CREAT | 0600);
    if (sem_id == -1) { perror("semget"); exit(1); }

    // ustawienie wartosci poczatkowych
    semctl(sem_id, SEM_BRIDGE, SETVAL, K);
    semctl(sem_id, SEM_MUTEX, SETVAL, 1);

    // kolejka komunikatow
    msg_id = msgget(key, IPC_CREAT | 0600);
    if (msg_id == -1) { perror("msgget"); exit(1); }
}

void ipc_attach(void) {
    key_t key = ftok("Makefile", 'A');

    shm_id = shmget(key, sizeof(shared_state_t), 0);
    if (shm_id == -1) { perror("child shmget"); exit(1); }
    state = shmat(shm_id, NULL, 0);

    sem_id = semget(key, SEM_COUNT, 0);
    msg_id = msgget(key, 0);
}

void ipc_detach(void) {
    shmdt(state);
}

void ipc_cleanup(void) {
    if (state) shmdt(state);
    if (shm_id != -1) shmctl(shm_id, IPC_RMID, NULL);
    if (sem_id != -1) semctl(sem_id, 0, IPC_RMID);
    if (msg_id != -1) msgctl(msg_id, IPC_RMID, NULL);
    unlink(FIFO_NAME);
}

// Funkcje pomocnicze dla semaforow
void sem_lock(int sem_num) {
    struct sembuf op = {sem_num, -1, 0};
    if (semop(sem_id, &op, 1) == -1) {
        if (errno != EINTR) perror("sem_lock");
    }
}

void sem_unlock(int sem_num) {
    struct sembuf op = {sem_num, 1, 0};
    if (semop(sem_id, &op, 1) == -1) perror("sem_unlock");
}

void sem_wait_bridge(int weight) {
    struct sembuf op = {SEM_BRIDGE, -weight, 0};
    if (semop(sem_id, &op, 1) == -1) {
        if (errno != EINTR) perror("sem_wait_bridge");
    }
}

void sem_signal_bridge(int weight) {
    struct sembuf op = {SEM_BRIDGE, weight, 0};
    if (semop(sem_id, &op, 1) == -1) perror("sem_signal_bridge");
}
