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

    int trips_needed = 2;

    log_msg("PASAZER %d: Port: %s%s", my_pid,
            (my_port == KRAKOW) ? "KRAKOW" : "TYNIEC",
            has_bike ? " (rower)" : "");

    while (trips_needed > 0) {

        int waiting_for_ship = 1;
        while (waiting_for_ship) {
            sem_lock(SEM_MUTEX);
            if (state->ship_state == FINISHED) {
                sem_unlock(SEM_MUTEX);
                ipc_detach();
                return 0;
            }

            // Sprawdzenie czy statek znajduje sie w moim porcie
            if (state->current_port == my_port &&
                state->ship_state == LOADING &&
                !state->boarding_closed) {
                waiting_for_ship = 0;
            }
            sem_unlock(SEM_MUTEX);

            if (waiting_for_ship) {
                // Jesli statek nie znajduje sie w moim porcie - poczekaj dluzej
//                usleep(50000);
            }
        }

        // Zajmowanie miejsca na moscie
        sem_wait_bridge(weight);

        // Rejestracja na moscie
        sem_lock(SEM_MUTEX);

        if (state->ship_state != LOADING || state->boarding_closed) {
            // Zaladunek zakonczony
            sem_unlock(SEM_MUTEX);
            sem_signal_bridge(weight);
            continue;
        }

        if (state->stack_top < state->K) {
            state->bridge_stack[state->stack_top++] = my_pid;
        }

        int boarded = 0;

        if (state->passengers_on_ship < state->N) {
            if (!has_bike || (has_bike && state->bikes_on_ship < state->M)) {

            // Wejscie na statek
            state->passengers_on_ship++;
            if (has_bike) state->bikes_on_ship++;
            boarded = 1;

            for(int i=0; i<state->stack_top; i++) {
                if(state->bridge_stack[i] == my_pid) {
                    for(int j=i; j<state->stack_top-1; j++) {
                        state->bridge_stack[j] = state->bridge_stack[j+1];
                    }
                    state->stack_top--;
                    break;
                }
             }
          }
        }

        sem_unlock(SEM_MUTEX);

        // Obsluga wyniku
        if (boarded) {
            // Zwolnienie miejsca na mostku
            sem_signal_bridge(weight);

            struct sembuf wait_disembark = {SEM_DISEMBARK, -1, 0};
            semop(sem_id, &wait_disembark, 1);

            // Opuszczenie statku
            my_port = (my_port == KRAKOW) ? TYNIEC : KRAKOW;
            trips_needed--;

            if (trips_needed > 0) {
  //              usleep(200000);
            }
        } else {

            while (1) {
                sem_lock(SEM_MUTEX);

                if (state->stack_top > 0 &&
                    state->bridge_stack[state->stack_top - 1] == my_pid) {

                    state->stack_top--;

                    if (state->stack_top == 0) sem_op(SEM_BRIDGE_EMPTY, 1);
                    else sem_op(SEM_LIFO_NOTIFY, 1);

                    sem_unlock(SEM_MUTEX);
                    break;
               }
               sem_unlock(SEM_MUTEX);
               struct sembuf wait_lifo = {SEM_LIFO_NOTIFY, -1, 0};
               semop(sem_id, &wait_lifo, 1);
        }
        sem_signal_bridge(weight);
    //    usleep(100000);
      }
   }

   log_msg("Pasazer %d ukonczyl rejsy", my_pid);
   ipc_detach(); 
   return 0;
}
