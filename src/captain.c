#define _POSIX_C_SOURCE 200809L
#include "ipc.h"
#include "log.h"
#include <unistd.h>
#include <signal.h>
#include <string.h>

int main(void) {
    ipc_attach();

    // Ustawienie handlerow sygnalow
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    sigprocmask(SIG_BLOCK, &set, NULL);

    // Rejestracja PID
    sem_lock(SEM_MUTEX);
    state->captain_pid = getpid();
    state->current_port = KRAKOW;  // Zaczynamy z Krakowa
    log_msg("KAPITAN: Rozpoczynam prace. PID=%d, Parametry: N=%d M=%d K=%d T1=%d T2=%d R=%d",
           getpid(), state->N, state->M, state->K, state->T1, state->T2, state->R);
    sem_unlock(SEM_MUTEX);

    // Glowna petla pracy kapitana
    while (1) {
        sem_lock(SEM_MUTEX);

        // Sprawdzenie czy osiagnieto limit rejsow
        if (state->trip_count >= state->R) {
            state->ship_state = FINISHED;
            log_msg("KAPITAN: Osiagnieto limit %d rejsow - KONIEC PRACY", state->R);
            sem_unlock(SEM_MUTEX);
            break;
        }

        // Inicjalizacja nowego rejsu
        state->ship_state = LOADING;
        state->boarding_closed = 0;
        state->stack_top = 0;
        state->passengers_on_ship = 0;
        state->bikes_on_ship = 0;

        log_msg("KAPITAN:  REJS %d/%d  Port: %s",
               state->trip_count + 1, state->R,
               (state->current_port == KRAKOW) ? "KRAKOW" : "TYNIEC");

        log_msg("KAPITAN: Rozpoczynam zaladunek (T1=%d sekund)", state->T1);
        sem_unlock(SEM_MUTEX);

        // Oczekiwanie na zaladunek: T1 lub SIGUSR1
        alarm(state->T1);
        int sig;
        int stop_received = 0;
        int early_depart = 0;

        while (!stop_received && !early_depart) {
            int ret = sigwait(&set, &sig);
            if (ret == 0) {
                if (sig == SIGALRM) {
                    log_msg("KAPITAN: Czas zaladunku (T1=%d) ", state->T1);
                    break;
                }
                if (sig == SIGUSR1) {
                    log_msg("KAPITAN: Otrzymano SIGUSR1");
                    early_depart = 1;
                    break;
                }
                if (sig == SIGUSR2) {
                    log_msg("KAPITAN: Otrzymano SIGUSR2");
                    stop_received = 1;
                }
            }
        }
        alarm(0);

        // Zakonczenie zaladunku
        sem_lock(SEM_MUTEX);
        state->ship_state = DEPARTING;
        state->boarding_closed = 1;

        log_msg("KAPITAN: Zamykam wejscie. Zaladowano: %d pasazerow, %d rowerow",
               state->passengers_on_ship, state->bikes_on_ship);

        // Sprawdzenie czy otrzymano SIGUSR2 podczas zaladunku
        if (stop_received) {
            log_msg("KAPITAN: Otrzymano STOP podczas zaladunku.");
            state->ship_state = FINISHED;
            sem_unlock(SEM_MUTEX);

            if (state->passengers_on_ship > 0) {
                log_msg("KAPITAN: Pasazerowie opuszczaja statek...");
                struct sembuf disembark_all = {SEM_DISEMBARK, state->passengers_on_ship, 0};
                semop(sem_id, &disembark_all, 1);
                custom_sleep(2);
            }
            break;
        }
        if (state->stack_top > 0) {
            log_msg("KAPITAN: Na moscie pozostalo %d osob. Oprozniam LIFO...",
                   state->stack_top);

           for (int i = 0; i < state->stack_top; i++) {
                struct sembuf wake_lifo = {SEM_LIFO_NOTIFY, 1, 0};
                semop(sem_id, &wake_lifo, 1);
            }
            sem_unlock(SEM_MUTEX);

            log_msg("KAPITAN: Czekam na oproznienie mostka...");
            struct sembuf wait_empty = {SEM_BRIDGE_EMPTY, -1, 0};
            semop(sem_id, &wait_empty, 1);

            log_msg("KAPITAN: Mostek pusty");

            // Ponowna blokada mutexu
            sem_lock(SEM_MUTEX);
        } else {
            log_msg("KAPITAN: Mostek pusty - gotowy do odjazdu");
        }

        if (state->passengers_on_ship == 0) {
            log_msg("KAPITAN: Brak pasażerów - rejs się nie odbywa");
            state->trip_count++;
            state->current_port = (state->current_port == KRAKOW) ? TYNIEC : KRAKOW;
            sem_unlock(SEM_MUTEX);
            log_msg("KAPITAN: Przechodzimy do portu %s",
                   (state->current_port == KRAKOW) ? "KRAKOW" : "TYNIEC");
            custom_sleep(1);
            continue;
        }
        // Rejs
        state->ship_state = SAILING;
        log_msg("KAPITAN: OdplywamyY z %s do %s! Na pokladzie: %d pasazerow, %d rowerow",
               (state->current_port == KRAKOW) ? "KRAKOW" : "TYNIEC",
               (state->current_port == KRAKOW) ? "TYNCA" : "KRAKOWA",
               state->passengers_on_ship, state->bikes_on_ship);
        sem_unlock(SEM_MUTEX);

        // Symulacja rejsu
        log_msg("KAPITAN: Rejs trwa %d sekund...", state->T2);
        custom_sleep(state->T2);

        // Wyladunek
        sem_lock(SEM_MUTEX);
        state->ship_state = UNLOADING;

        port_t old_port = state->current_port;
        state->current_port = (state->current_port == KRAKOW) ? TYNIEC : KRAKOW;
        int p_count = state->passengers_on_ship;
        int b_count = state->bikes_on_ship;
        log_msg("KAPITAN: Doplynelismy do %s. Wysiadka %d pasazerow, %d rowerow",
               (state->current_port == KRAKOW) ? "KRAKOWA" : "TYNCA",
               p_count, b_count);
        sem_unlock(SEM_MUTEX);
        if (p_count > 0) {
            struct sembuf op_disembark = {SEM_DISEMBARK, p_count, 0};
            semop(sem_id, &op_disembark, 1);
            // Daj czas na wysiadkę
            custom_sleep(2);
        }
        sem_lock(SEM_MUTEX);
        state->passengers_on_ship = 0;
        state->bikes_on_ship = 0;
        state->trip_count++;
        log_msg("KAPITAN: Rejs %d zakonczony. Przeplynieto: %s -> %s",
               state->trip_count,
               (old_port == KRAKOW) ? "KRAKOW" : "TYNIEC",
               (state->current_port == KRAKOW) ? "KRAKOW" : "TYNIEC");

        log_msg("KAPITAN: Pozostalo rejsow: %d", state->R - state->trip_count);
        sem_unlock(SEM_MUTEX);

    }

    log_msg("KAPITAN: Koniec pracy. Wykonano %d rejsow.", state->trip_count);
    ipc_detach();
    return 0;
}
