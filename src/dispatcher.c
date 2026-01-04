#include "common.h"
#include "log.h"
#include <time.h>
#include <signal.h>
#include <unistd.h>

int main(void) {
   srand(getpid() ^ time(NULL));

   key_t key = ftok(".", 'S');
   shm_id = shmget(key, sizeof(shared_state_t), 0);
   sem_id = semget(key, SEM_COUNT, 0);
   state = shmat(shm_id, NULL, 0);

   log_event("DYSPOZYTOR: Uruchomiony");

   while (1) {
       sleep(rand() % 8 + 3);

       sem_lock(sem_id, SEM_STATE);

       if (state->ship_state == FINISHED) {
           sem_unlock(sem_id, SEM_STATE);
           break;
       }

       pid_t captain_pid = state->captain_pid;
       ship_state_t st = state->ship_state;
       sem_unlock(sem_id, SEM_STATE);

       if (st == LOADING && (rand() % 2)) {
           log_event("DYSPOZYTOR: Wysylam SIGUSR1");
           kill(captain_pid, SIGUSR1);
       }

       if (rand() % 10 == 0) {
           log_event("DYSPOZYTOR: Wysylam SIGUSR2");
           kill(captain_pid, SIGUSR2);
           break;
       }
   }

   log_event("DYSPOZYTOR: Koncze prace");
   shmdt(state);
   return 0;
}

