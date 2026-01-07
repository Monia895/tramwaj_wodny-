#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "ipc.h"
#include "log.h"
#include "msg.h"
#include <sys/msg.h>
#include <unistd.h>

volatile sig_atomic_t signal_early = 0;
volatile sig_atomic_t signal_stop = 0;

void handle_sig(int sig) {
    if (sig == SIGUSR1) signal_early = 1;
    if (sig == SIGUSR2) signal_stop = 1;
}

int main(void) {
    ipc_attach();

    struct sigaction sa;
    sa.sa_handler = handle_sig;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);

    sem_lock(SEM_MUTEX);
    state->captain_pid = getpid();
    sem_unlock(SEM_MUTEX);

    log_msg("KAPITAN: Rozpoczynam prace. PID=%d", getpid());

    while (1) {
        // sprawdzenie konca pracy
        sem_lock(SEM_MUTEX);
        if (state->trip_count >= state->R || signal_stop) {
            state->ship_state = FINISHED;
            sem_unlock(SEM_MUTEX);
            break;
        }

        // zaladunek
        state->ship_state = LOADING;
        state->boarding_closed = 0;
        state->stack_top = 0;
        int trip = state->trip_count + 1;
        sem_unlock(SEM_MUTEX);

        log_msg("KAPITAN: Rejs %d - Otwieram wejscie (T1=%ds)", trip, state->T1);

        // oczekiwanie T1 lub sygnal
        int time_waited = 0;
        signal_early = 0;

        while (time_waited < state->T1 && !signal_early && !signal_stop) {
            sleep(1);
            time_waited++;

            // obsluga kolejki komunikatow
            struct msg_buf msg;
            if (msgrcv(msg_id, &msg, sizeof(msg) - sizeof(long), 0, IPC_NOWAIT) != -1) {
                if (msg.cmd == 1) signal_early = 1;
                if (msg.cmd == 2) signal_stop = 1;
            }
        }

        if (signal_stop) continue;

        // odplywanie
        sem_lock(SEM_MUTEX);
        state->ship_state = DEPARTING;
        state->boarding_closed = 1;
        sem_unlock(SEM_MUTEX);

        log_msg("KAPITAN: Koniec zaladunku! Prosze opuscic mostek.");

        // czekanie az mostek bedzie pusty
        while (1) {
            sem_lock(SEM_MUTEX);
            int on_bridge = state->stack_top;
            sem_unlock(SEM_MUTEX);

            if (on_bridge == 0) break;
            usleep(100000);
        }

        // rejs
        sem_lock(SEM_MUTEX);
        state->ship_state = SAILING;
        log_msg("KAPITAN: Wyplywamy! Pasazerow: %d, Rowerow: %d",
              state->passengers_on_ship, state->bikes_on_ship);
        sem_unlock(SEM_MUTEX);

        sleep(state->T2);

        // wyladunek
        sem_lock(SEM_MUTEX);
        state->ship_state = UNLOADING;
        state->current_port = (state->current_port == KRAKOW) ? TYNIEC : KRAKOW;
        log_msg("KAPITAN: Doplynelismy do %s. Wyladunek.", 
               state->current_port == KRAKOW ? "Krakowa" : "Tynca");

        // reset stanu statku
        state->passengers_on_ship = 0;
        state->bikes_on_ship = 0;
        state->trip_count++;
        sem_unlock(SEM_MUTEX);

        sleep(2);
    }

    log_msg("KAPITAN: Koniec pracy.");
    ipc_detach();
    return 0;
}
