#ifndef IPC_H
#define IPC_H

#include "common.h"

// funcje zarzadzania zasobami
void ipc_init_all(int N, int M, int K, int T1, int T2, int R);
void ipc_attach(void);
void ipc_cleanup(void);
void ipc_detach(void);

// pomocnicze funkcje semaforow
void sem_lock(int sem_num);
void sem_unlock(int sem_num);
void sem_wait_bridge(int weight);
void sem_signal_bridge(int weight);
void sem_op(int sem_num, int op_val);

void custom_sleep(int t);

#endif
