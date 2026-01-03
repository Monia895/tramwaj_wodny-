#include "common.h"
#include "log.h"
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

/* flagi sygnalow */
volatile sig_atomic_t early_departure = 0;
volatile sig_atomic_t end_of_service = 0;

void handle_signal(int sig) {
    if (sig == SIGUSR1)
       early_departure = 1;
    else if (sig == SIGUSR2)
       end_of_service = 1;
}

int main(void) {

    key_t key = ftok(".", 'S');
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    shm_id = shmget(key, sizeof(shared_state_t), 0);
    if (shm_id == -1) {
        perror("shmget");
        exit(1);
    }

    sem_id = semget(key, 1, 0);
    if (sem_id == -1) {
        perror("semget");
        exit(1);
    }

    state = shmat(shm_id, NULL, 0);
    if (state == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    /* obsluga sygnalow */
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGUSR1, &SA, NULL);
    sigaction(SIGUSR2, &SA, NULL);

    log_event("KAPITAN: Rozpoczynam prace");

    int trip_count = 0;

    while (trip_count < R && !end_of_service) {

        /* zaladunek */
        sem_lock(sem_id);
        state->ship_state = LOADING;
        sem_unlock(sem_id);

        log_event("KAPITAN: Rozpoczeto zaladunek rejsu %d", trip_count + 1);

        /* oczekiwanie T1 lub SIGUSR1 */
        early_departure = 0;
        int time_left = T1;
        while (time_left > 0 && !early_departure && !end_of_service) {
            time_left = sleep(time_left);
        }

        /* zamykanie wejscia */
        sem_lock(sem_id);
        state->ship_state = DEPARTING;
        sem_unlock(sem_id);

        log_event("KAPITAN: Zamykanie wejscia, oczekiwanie na pusty mostek");

        /* czekanie az mostek bedzie pusty */
        while (1) {
            sem_lock(sem_id);
            int on_bridge = state->passengers_on_bridge;
            sem_unlock(sem_id);

            if (on_bridge == 0)
                break;

            usleeep(100000);
        }

        /* rejs */
        sem_lock(sem_id);
        state->ship_state = SAILING;
        sem_unlock(sem_id);

        log_event("KAPITAN: Rejs rozpoczety (pasazerowie=%d, rowery=%d)",
                  state->passengers_on_ship,
                  state->bikes_on_ship;

        sleep(T2);

        /* wyladunek */
        sem_lock(sem_id);
        state->ship_state = UNLOADING;

        state->passengers_on_ship = 0;
        state->bikes_on_ship = 0;
        sem_unlock(sem_id);

        log_event("KAPITAN: Wyladunek zakonczony");

        trip_count++;
    }

    /* koniec pracy */
    sem_lock(sem_id);
    state->ship_state = FINISHED;
    sem_unlock(sem_id);

    log_event("KAPITAN: Zakonczono prace po %d rejsach", trip_count);

    shmdt(state);
    return 0;
}
