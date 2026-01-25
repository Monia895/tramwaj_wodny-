#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "ipc.h"
#include "log.h"
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

int main(void) {
    srand(getpid() ^ time(NULL));
    ipc_attach();

    // losowanie atrybutow pasazera
    pid_t my_pid = getpid();
    port_t my_port = (rand() % 2 == 0) ? KRAKOW : TYNIEC;
    int has_bike = (rand() % 100 < 30);
    int weight = has_bike ? 2 : 1;

    log_msg("PASAZER %d: Pojawil sie w porcie: %s%s", my_pid,
            (my_port == KRAKOW) ? "KRAKOW" : "TYNIEC",
            has_bike ? " (z rowerem)" : "");

    while (1) {
        usleep(50000 + rand() % 150000);

        sem_lock(SEM_MUTEX);

        if (state->ship_state == FINISHED) {
            sem_unlock(SEM_MUTEX);
            log_msg("PASAZER %d: Statek zakonczyl prace - koncze", my_pid);
            break;
        }

        // Sprawdzenie czy statek znajduje sie w moim porcie
        int can_try_enter = 0;
        if (state->current_port == my_port &&
            state->ship_state == LOADING &&
            !state->boarding_closed) {
            can_try_enter = 1;
        }

        sem_unlock(SEM_MUTEX);

        if (!can_try_enter) {
            // Jesli statek nie znajduje sie w moim porcie - poczekaj dluzej
            usleep(500000 + rand() % 1000000);
            continue;
        }

        int bridge_space;
        do {
            bridge_space = semctl(sem_id, SEM_BRIDGE, GETVAL, 0);
        } while (bridge_space == -1 && errno == EINTR);

        if (bridge_space < weight) {

            usleep(100000 + rand() % 200000);
            continue;
        }

        // Zajmowanie miejsca na moscie
        sem_wait_bridge(weight);

        // Rejestracja na moscie
        sem_lock(SEM_MUTEX);

        if (state->ship_state != LOADING || state->boarding_closed) {
            // Zaladunek zakonczony
            sem_unlock(SEM_MUTEX);
            sem_signal_bridge(weight);
            usleep(100000);
            continue;
        }

        if (state->stack_top >= state->K) {
            sem_unlock(SEM_MUTEX);
            sem_signal_bridge(weight);
            usleep(100000);
            continue;
        }
        state->bridge_stack[state->stack_top] = my_pid;
        state->stack_top++;
        log_msg("PASAZER %d: Na moscie (poz: %d/%d)%s", 
               my_pid, state->stack_top, state->K,
               has_bike ? " z rowerem" : "");

        int boarded = 0;

        if (state->passengers_on_ship < state->N &&
            (!has_bike || state->bikes_on_ship < state->M)) {

            // Wejscie na statek
            state->passengers_on_ship++;
            if (has_bike) state->bikes_on_ship++;
            boarded = 1;

            for(int i = 0; i < state->stack_top; i++) {
                if(state->bridge_stack[i] == my_pid) {
                    for(int j = i; j < state->stack_top - 1; j++) {
                        state->bridge_stack[j] = state->bridge_stack[j+1];
                    }
                    state->stack_top--;
                    break;
                }
            }

            log_msg("PASAZER %d: Wszedl na statek! Stat: %d/%d pax, %d/%d row",
                   my_pid,
                   state->passengers_on_ship, state->N,
                   state->bikes_on_ship, state->M);
        } else {
            log_msg("PASAZER %d: Brak miejsca na statku (pax: %d/%d, row: %d/%d)",
                   my_pid,
                   state->passengers_on_ship, state->N,
                   state->bikes_on_ship, state->M);
        }

        sem_unlock(SEM_MUTEX);

        // Obsluga wyniku
        if (boarded) {
            // Zwolnienie miejsca na mostku
            sem_signal_bridge(weight);

            log_msg("PASAZER %d: Czeka na rejs...", my_pid);
            struct sembuf wait_disembark = {SEM_DISEMBARK, -1, 0};
            semop(sem_id, &wait_disembark, 1);

            // Opuszczenie statku
            my_port = (my_port == KRAKOW) ? TYNIEC : KRAKOW;
            log_msg("PASAZER %d: Wysiadl w: %s", my_pid,
                    (my_port == KRAKOW) ? "KRAKOW" : "TYNIEC");

            // Decyzja czy wraca
            int will_return = (rand() % 100 < 50);
            if (!will_return) {
                log_msg("PASAZER %d: Idzie do domu.", my_pid);
                break;
            }

            log_msg("PASAZER %d: Czeka na powrot w %s", my_pid,
                    (my_port == KRAKOW) ? "KRAKOW" : "TYNIEC");

            usleep(500000 + rand() % 1000000);
        } else {
            log_msg("PASAZER %d: Wycofuje sie z mostka...", my_pid);
            while (1) {
                sem_lock(SEM_MUTEX);

                if (state->stack_top > 0 &&
                    state->bridge_stack[state->stack_top - 1] == my_pid) {

                    state->stack_top--;
                    int remaining = state->stack_top;

                    log_msg("PASAZER %d: Schodzi z mostka (LIFO). Pozostalo: %d",
                           my_pid, remaining);

                    if (remaining == 0) {
                        struct sembuf notify_cap = {SEM_BRIDGE_EMPTY, 1, 0};
                        semop(sem_id, &notify_cap, 1);
                    } else {
                        struct sembuf notify_next = {SEM_LIFO_NOTIFY, 1, 0};
                        semop(sem_id, &notify_next, 1);
                    }
                    sem_unlock(SEM_MUTEX);
                    break;
                }
                sem_unlock(SEM_MUTEX);

                struct sembuf wait_lifo = {SEM_LIFO_NOTIFY, -1, 0};
                semop(sem_id, &wait_lifo, 1);
            }
            sem_signal_bridge(weight);

            usleep(300000 + rand() % 500000);
        }
    }

    log_msg("PASAZER %d: Koniec podrozy", my_pid);
    ipc_detach();
    return 0;
}
