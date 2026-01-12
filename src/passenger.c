#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "ipc.h"
#include "log.h"
#include <time.h>
#include <unistd.h>

int main(void) {
    srand(getpid() ^ time(NULL));
    ipc_attach();

    // losowanie atrybutow pasazera
    pid_t my_pid = getpid();
    int has_bike = (rand() % 100 < 30);
    int weight = has_bike ? 2 : 1;

    // czekanie na statek
    while (1) {
        struct sembuf wait_gate = {SEM_ENTRY_GATE, -1, 0};
        if (semop(sem_id, &wait_gate, 1) == -1) {
             if (errno == EINTR) continue;
             return 0;
        }

        sem_lock(SEM_MUTEX);
        if (state->ship_state == FINISHED) {
            sem_unlock(SEM_MUTEX);
            ipc_detach();
            return 0;
        }
        if (state->ship_state == LOADING && !state->boarding_closed) {
            sem_unlock(SEM_MUTEX);
            break;
        }
        sem_unlock(SEM_MUTEX);
    }

    // wejscie na mostek
    sem_wait_bridge(weight);

    sem_lock(SEM_MUTEX);
    if (state->boarding_closed || state->ship_state != LOADING) {
        sem_unlock(SEM_MUTEX);
        sem_signal_bridge(weight);
        ipc_detach();
        return 0;
    }

    int my_stack_idx = state->stack_top;
    if (my_stack_idx < MAX_K) {
        state->bridge_stack[my_stack_idx] = my_pid;
        state->stack_top++;
    }

    log_msg("PASAZER %d: Na mostku (Rower: %d)", my_pid, has_bike);
    sem_unlock(SEM_MUTEX);

    // proba wejscia na poklad
    int boarded = 0;
    sem_lock(SEM_MUTEX);
    if (!state->boarding_closed && state->ship_state == LOADING) {
        // sprawdzenie limitow
        if (state->passengers_on_ship < state->N &&
           (!has_bike || state->bikes_on_ship < state->M)) {

            state->passengers_on_ship++;
            if (has_bike) state->bikes_on_ship++;
            boarded = 1;

            // usuniecie ze stosu i przesuniecie reszty
            for(int i=0; i<state->stack_top; i++) {
                if(state->bridge_stack[i] == my_pid) {
                    for(int j=i; j<state->stack_top-1; j++) {
                        state->bridge_stack[j] = state->bridge_stack[j+1];
                    }
                    state->stack_top--;
                    break;
                }
            }

            log_msg("PASAZER %d: Wszedl na statek.", my_pid);
        } else {
            log_msg("PASAZER %d: Brak miejsca!", my_pid);
        }
    } else {
        log_msg("PASAZER %d: Wejscie zamkniete!", my_pid);
    }
    sem_unlock(SEM_MUTEX);

    // jesli nie udalo sie wejss -> wycofanie
    if (!boarded) {
        while (1) {
            sem_lock(SEM_MUTEX);
            if (state->stack_top > 0 && state->bridge_stack[state->stack_top - 1] == my_pid) {
                state->stack_top--;
                int remaining = state->stack_top;
                log_msg("PASAZER %d: Schodzi z mostka (LIFO).", my_pid);

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
        ipc_detach();
        return 0;
    }

    sem_signal_bridge(weight);

    // rejs
    struct sembuf wait_disembark = {SEM_DISEMBARK, -1, 0};
    semop(sem_id, &wait_disembark, 1);

    sem_lock(SEM_MUTEX);
    log_msg("PASAZER %d: Wysiada.", my_pid);
    sem_unlock(SEM_MUTEX);

    ipc_detach();
    return 0;
}

