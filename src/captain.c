#define _POSIX_C_SOURCE 200809L

#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/msg.h>
#include <string.h>

#include "common.h"
#include "log.h"
#include "msg.h"

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

    sem_id = semget(key, SEM_COUNT, 0);
    if (sem_id == -1) {
        perror("semget");
        exit(1);
    }

    state = shmat(shm_id, NULL, 0);
    if (state == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    extern int msg_id;

    /* obsluga sygnalow */
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);

    log_event("KAPITAN: Rozpoczynam prace");

   sem_lock(sem_id, SEM_STATE);
   state->captain_pid = getpid();
   state->trip_count = 0;
   sem_unlock(sem_id, SEM_STATE);

    while (!end_of_service) {

        struct msg m;
        if (msgrcv(msg_id, &m, sizeof(m.text), 0, IPC_NOWAIT) != -1) {

            if (strcmp(m.text, "EARLY_DEPARTURE") == 0) {
                early_departure = 1;
                log_event("KAPITAN: Polecenie z kolejki – wczesny wyplyw");
            }

            if (strcmp(m.text, "STOP") == 0) {
                end_of_service = 1;
                log_event("KAPITAN: Polecenie z kolejki – koniec pracy");
            }
        }


        sem_lock(sem_id, SEM_STATE);
        if (state->trip_count >= R) {
            sem_unlock(sem_id, SEM_STATE);
            break;
        }
        sem_unlock(sem_id, SEM_STATE);

        /* zaladunek */
        sem_lock(sem_id, SEM_STATE);
        state->bridge_dir = TO_SHIP;
        state->ship_state = LOADING;
        sem_unlock(sem_id, SEM_STATE);

        log_event("KAPITAN: Rozpoczeto zaladunek rejsu %d", trip_count + 1);

        /* oczekiwanie T1 lub SIGUSR1 */
        early_departure = 0;
        int time_left = T1;
        while (time_left > 0 && !early_departure && !end_of_service) {
            time_left = sleep(time_left);
        }

        sem_lock(sem_id, SEM_STATE);
        if (end_of_service && state->ship_state = LOADING) {
            state->passengers_on_ship = 0;
            state->bikes_on_ship = 0;
            sem_unlock(sem_id, SEM_STATE);
            log_event("KAPITAN: SIGUSR2 podczas zaladunku - pasazerowie opuszczaja statek");
            break;
        }
        sem_unlock(sem_id, SEM_STATE);

        /* zamykanie wejscia */
        sem_lock(sem_id, SEM_STATE);
        state->ship_state = DEPARTING;
        state->boarding_closed = 1;
        sem_unlock(sem_id, SEM_STATE);

        log_event("KAPITAN: Zamykanie wejscia, oczekiwanie na pusty mostek");

        /* czekanie az mostek bedzie pusty */
        while (1) {
            sem_lock(sem_id, SEM_STATE);
            int on_bridge = state->passengers_on_bridge;
            sem_unlock(sem_id, SEM_STATE);

            if (on_bridge == 0)
                break;

            usleep(100000);
        }

        /* rejs */
        sem_lock(sem_id, SEM_STATE);
        state->ship_state = SAILING;
        sem_unlock(sem_id, SEM_STATE);

        log_event("KAPITAN: Rejs rozpoczety (pasazerowie=%d, rowery=%d)",
                  state->passengers_on_ship,
                  state->bikes_on_ship);

        sleep(T2);

        /* wyladunek */
        sem_lock(sem_id, SEM_STATE);
        state->bridge_dir = TO_SHIP;
        state->ship_state = UNLOADING;

        state->passengers_on_ship = 0;
        state->bikes_on_ship = 0;

        state->current_port =
            (state->current_port == KRAKOW) ? TYNIEC : KRAKOW;

        sem_unlock(sem_id, SEM_STATE);

        log_event("KAPITAN: Doplynelismy do %s",
            state->current_port == KRAKOW ? "KRAKOW" : "TYNIEC");

        sem_lock(sem_id, SEM_STATE);
        state->trip_count++;
        sem_unlock(sem_id, SEM_STATE);
    }

    /* koniec pracy */
    sem_lock(sem_id, SEM_STATE);
    state->ship_state = FINISHED;
    sem_unlock(sem_id, SEM_STATE);

    sem_lock(sem_id, SEM_STATE);
    int finished_trips = state->trip_count;
    sem_unlock(sem_id, SEM_STATE);

    log_event("KAPITAN: Zakonczono prace po %d rejsach", trip_count);

    shmdt(state);
    return 0;
}
