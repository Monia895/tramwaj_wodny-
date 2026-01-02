#include "common.h"
#include "log.h"

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    key_t key = ftok(".", 'S');
    shm_id = shmget(key, sizeof(shared_state_t), 0);
    if (shm_id == -1) {
        perror("shmget passenger");
        exit(1);
    }

    sem_id = semget(key, 1, 0);
    if (sem_id == -1) {
        perror("semget passenger");
        exit(1);
    }

    state = shmat(shm_id, NULL, 0);

    log_event("PASAZER [%d]: wszedl na mostek", getpid());

    sem_lock(sem_id);
    state->passengers_on_bridge++;
    printf("PASAZER: wszedl na mostek (na mostku: %d)\n", 
           state->passengers_on_bridge);
    sem_unlock(sem_id);

    shmdt(state);
    return 0;
}

