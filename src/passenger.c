#include "common.h"

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    key_t key = ftok(".", 'S');
    shm_id = shmget(key, sizeof(shared_state_t), 0);
    shm_id = semget(key, 1, 0);

    state = shmat(shm_id, NULL, 0);

    sem_lock(sem_id);
    state->passengers_on_bridge++;
    printf("PASAZER: wszedl na mostek (na mostku: %d)\n", 
           state->passengers_on_bridge);
    sem_unlock(sem_id);

    shmdt(state);
    return 0;
}

