#include "ipc.h"
#include "msg.h"
#include <sys/msg.h>
#include <string.h>

// zmienne globalne
int shm_id = -1;
int sem_id = -1;
int msg_id = -1;
shared_state_t *state = NULL;

void custom_sleep(int t) {
    if (t > 0) {
//        usleep(t * 1000000);
    }
}

// tworzenie wszystkich zasobow IPC
void ipc_init_all(int N, int M, int K, int T1, int T2, int R) {
    key_t key = ftok("Makefile", 'A');
    if (key == -1) { perror("ftok"); exit(1); }

    // tworzenie pamieci dzielonej
    shm_id = shmget(key, sizeof(shared_state_t), IPC_CREAT | 0600);
    if (shm_id == -1) { perror("shmget"); exit(1); }

    state = shmat(shm_id, NULL, 0);
    if (state == (void*)-1) { perror("shmat"); exit(1); }

    // zerowanie pamieci
    memset(state, 0, sizeof(shared_state_t));
    state->N = N; state->M = M; state->K = K;
    state->T1 = T1; state->T2 = T2; state->R = R;

    // tworzenie semaforow
    sem_id = semget(key, SEM_COUNT, IPC_CREAT | 0600);
    if (sem_id == -1) { perror("semget"); exit(1); }

    // ustawienie wartosci poczatkowych
    semctl(sem_id, SEM_BRIDGE, SETVAL, K);
    semctl(sem_id, SEM_MUTEX, SETVAL, 1);
    semctl(sem_id, SEM_ENTRY_GATE, SETVAL, 0);
    semctl(sem_id, SEM_LIFO_NOTIFY, SETVAL, 0);
    semctl(sem_id, SEM_BRIDGE_EMPTY, SETVAL, 0);
    semctl(sem_id, SEM_DISEMBARK, SETVAL, 0);

    // tworzenie kolejki komunikatow
    msg_id = msgget(key, IPC_CREAT | 0600);
    if (msg_id == -1) { perror("msgget"); exit(1); }
}

// podlaczanie procesu do istniejacych zasobow
void ipc_attach(void) {
    key_t key = ftok("Makefile", 'A');

    shm_id = shmget(key, sizeof(shared_state_t), 0);
    state = shmat(shm_id, NULL, 0);
    sem_id = semget(key, SEM_COUNT, 0);
    msg_id = msgget(key, 0);
}

// odlaczanie pamieci
void ipc_detach(void) {
    shmdt(state);
}

// calkowite usuwanie zasobow z systemu
void ipc_cleanup(void) {
    if (state) shmdt(state);
    if (shm_id != -1) shmctl(shm_id, IPC_RMID, NULL);
    if (sem_id != -1) semctl(sem_id, 0, IPC_RMID);
    if (msg_id != -1) msgctl(msg_id, IPC_RMID, NULL);
    unlink("/tmp/tram_log_fifo");
}

void sem_op(int sem_num, int op_val) {
    struct sembuf op = {sem_num, op_val, 0};
    while (semop(sem_id, &op, 1) == -1) {
        if (errno == EIDRM || errno == EINVAL) exit(0);
        if (errno != EINTR) { perror("semop"); exit(1); }
    }
}

void sem_lock(int sem_num) { sem_op(sem_num, -1); }
void sem_unlock(int sem_num) { sem_op(sem_num, 1); }
void sem_wait_bridge(int weight) { sem_op(SEM_BRIDGE, -weight); }
void sem_signal_bridge(int weight) { sem_op(SEM_BRIDGE, weight); }
