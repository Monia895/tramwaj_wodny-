#include "ipc.h"
#include <sys/msg.h>

void ipc_init(int bridge_capacity) {
   key_t key = ftok(".", 'S');
   if (key == -1) {
      perror("ftok");
      exit(1);
   }

   msg_id = msgget(key, IPC_CREAT | 0600);
   if (msg_id == -1) {
       perror("msgget");
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

   sem_id = semget(key, SEM_COUNT, IPC_CREAT | 0600);
   if (sem_id == -1) {
       perror("semget");
       exit(1);
   }

   /* inicjalizacja semaforow */
   if (semctl(sem_id, SEM_BRIDGE, SETVAL, bridge_capacity) == -1)
       perror("semctl SEM_BRIDGE");
   if (semctl(sem_id, SEM_STATE,  SETVAL, 1) == -1)
       perror("semctl SEM_STATE");
   if (semctl(sem_id, SEM_SHIP,   SETVAL, 1) == -1)
       perror("semctl SEM_SHIP");
   if (semctl(sem_id, SEM_LOG,    SETVAL, 1) == -1)
       perror("semctl SEM_LOG");
}

void ipc_cleanup(void) {
    shmdt(state);
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);
    if (msg_id != -1)
        msgctl(msg_id, IPC_RMID, NULL);
}
