#include "ipc.h"

void ipc_init(void) {
   key_t key = ftok(".", 'S');
   if (key == -1) {
      perror("ftok");
      exit(1);
   }

   shm_id = shmget(key, sizeof(shared_state_t), IPC_CREAT | 0600);
   if (shm_id == -1) {
       perror("shmget");
       exit(1);
   }

   state = shmat(shm_id, NULL, 0);
   if (state == (void *)-1) {
       perror("shmat");
       exit(1);
   }

   sem_id = semget(key, 1, IPC_CREAT | 0600);
   if (sem_id == -1) {
       perror("semget");
       exit(1);
   }

   if (semctl(sem_id, 0, SETVAL, 1) == -1) {
       perror("semctl");
       exit(1);
   }
}

void ipc_cleanup(void) {
    shmdt(state);
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);
}
