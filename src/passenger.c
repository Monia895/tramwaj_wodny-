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

    // oczekiwanie na statek
    while (1) {
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
        usleep(200000);
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
    } else {
        sem_unlock(SEM_MUTEX);
        sem_signal_bridge(weight);
        ipc_detach();
        return 1;
    }

    log_msg("PASAZER %d: Wszedl na mostek (Rower: %s)", my_pid, has_bike ? "TAK" : "NIE");
    sem_unlock(SEM_MUTEX);

    // proba wejscia na poklad
    int boarded = 0;

    // symulacja przejscia przez mostek
    usleep(100000);

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
            log_msg("PASAZER %d: Brak miejsca na statku!", my_pid);
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
                sem_unlock(SEM_MUTEX);
                break;
            }
            sem_unlock(SEM_MUTEX);
            usleep(50000);
       }

        log_msg("PASAZER %d: Wycofuje sie z mostka (LIFO).", my_pid);
        sem_signal_bridge(weight);
        ipc_detach();
        return 0;
    }

    sem_signal_bridge(weight);

    // rejs statkiem
    while (1) {
        sem_lock(SEM_MUTEX);
        if (state->ship_state == UNLOADING) {
            sem_unlock(SEM_MUTEX);
            break;
        }
        sem_unlock(SEM_MUTEX);
        sleep(1);
    }

    log_msg("PASAZER %d: Opuszcza statek w porcie docelowym.", my_pid);
    ipc_detach();
    return 0;
}

