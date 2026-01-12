#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "ipc.h"
#include "log.h"
#include "msg.h"
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>

int main(void) {
    ipc_attach();

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    sigprocmask(SIG_BLOCK, &set, NULL);

    sem_lock(SEM_MUTEX);
    state->captain_pid = getpid();
    sem_unlock(SEM_MUTEX);

    log_msg("KAPITAN: Rozpoczynam prace. PID=%d", getpid());

    // glowna petla pracy kapitana
    while (1) {
        sem_lock(SEM_MUTEX);
        if (state->trip_count >= state->R) {
            state->ship_state = FINISHED;
            struct sembuf op = {SEM_ENTRY_GATE, 1000, 0};
            semop(sem_id, &op, 1);
            sem_unlock(SEM_MUTEX);
            break;
        }

        // zaladunek
        state->ship_state = LOADING;
        state->boarding_closed = 0;
        state->stack_top = 0;
        int trip = state->trip_count + 1;
        semctl(sem_id, SEM_BRIDGE_EMPTY, SETVAL, 0);
        sem_unlock(SEM_MUTEX);

        log_msg("KAPITAN: Rejs %d - Otwieram wejscie (T1=%ds)", trip, state->T1);

        // otwarcie bramki dla pasazerow
        struct sembuf open_gate = {SEM_ENTRY_GATE, 1000, 0};
        semop(sem_id, &open_gate, 1);

        // oczekiwanie T1 lub sygnal
        alarm(state->T1);
        int sig;
        int stop_received = 0;
        int early_depart = 0;

        while (!stop_received && !early_depart) {

            // obsluga kolejki komunikatow
            struct msg_buf msg;
            while (msgrcv(msg_id, &msg, sizeof(msg) - sizeof(long), 0, IPC_NOWAIT) != -1) {
                if (msg.cmd == 1) early_depart = 1;
                if (msg.cmd == 2) stop_received = 1;
            }
            if (stop_received || early_depart) break;

            sigwait(&set, &sig);
            if (sig == SIGALRM) break;
            if (sig == SIGUSR1) early_depart = 1;
            if (sig == SIGUSR2) stop_received = 1;
        }
        alarm(0);

        sem_lock(SEM_MUTEX);
        state->ship_state = DEPARTING;
        state->boarding_closed = 1;
        semctl(sem_id, SEM_ENTRY_GATE, SETVAL, 0);

        log_msg("KAPITAN: Koniec zaladunku! Prosze opuscic mostek.");

        // czekanie az mostek bedzie pusty
        if (state->stack_top > 0) {
            struct sembuf wake_lifo = {SEM_LIFO_NOTIFY, 1, 0};
            semop(sem_id, &wake_lifo, 1);

            sem_unlock(SEM_MUTEX);
            struct sembuf wait_empty = {SEM_BRIDGE_EMPTY, -1, 0};
            semop(sem_id, &wait_empty, 1);
        } else {
            sem_unlock(SEM_MUTEX);
        }

        if (stop_received) {
             log_msg("KAPITAN: Otrzymano STOP. Koncze rejsy.");
             sem_lock(SEM_MUTEX);
             state->ship_state = FINISHED;
             sem_unlock(SEM_MUTEX);
             break;
        }

        // rejs
        sem_lock(SEM_MUTEX);
        state->ship_state = SAILING;
        log_msg("KAPITAN: Wyplywamy! Pasazerow: %d, Rowerow: %d",
              state->passengers_on_ship, state->bikes_on_ship);
        sem_unlock(SEM_MUTEX);

        alarm(state->T2);
        sigwait(&set, &sig);
        alarm(0);

        // wyladunek
        sem_lock(SEM_MUTEX);
        state->ship_state = UNLOADING;
        state->current_port = (state->current_port == KRAKOW) ? TYNIEC : KRAKOW;
        log_msg("KAPITAN: Doplynelismy do %s. Wyladunek.", 
               state->current_port == KRAKOW ? "Krakowa" : "Tynca");

        int p_count = state->passengers_on_ship;
        sem_unlock(SEM_MUTEX);

        if (p_count > 0) {
            struct sembuf op_disembark = {SEM_DISEMBARK, p_count, 0};
            semop(sem_id, &op_disembark, 1);
        }
        alarm(2);
        sigwait(&set, &sig);

        sem_lock(SEM_MUTEX);
        state->passengers_on_ship = 0;
        state->bikes_on_ship = 0;
        state->trip_count++;
        sem_unlock(SEM_MUTEX);
    }

    log_msg("KAPITAN: Koniec pracy.");
    ipc_detach();
    return 0;
}
