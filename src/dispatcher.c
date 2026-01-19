#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "ipc.h"
#include "log.h"
#include "msg.h"
#include <time.h>
#include <signal.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>

void handler(int sig) { (void)sig; }

int main(void) {
    srand(time(NULL));
    ipc_attach();

    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    log_msg("DYSPOZYTOR: Start pracy.");

    int attempts = 0;
    const int MAX_ATTEMPTS = 50;

    while (attempts < MAX_ATTEMPTS) {
        attempts++;

        custom_sleep(rand() % 5 + 3);

        sem_lock(SEM_MUTEX);
        if (state->ship_state == FINISHED) {
            sem_unlock(SEM_MUTEX);
            break;
        }
        pid_t cap_pid = state->captain_pid;
        ship_state_t st = state->ship_state;
        sem_unlock(SEM_MUTEX);

        int action = rand() % 100;

        if (action < 15 && st == LOADING) {
            // sygnal 1: Odplywaj wczesniej (SIGUSR1)
            log_msg("DYSPOZYTOR: Nakaz wczesniejszego wyplyniecia (SIGUSR1)");
            kill(cap_pid, SIGUSR1);
        }
        else if (action >= 98) {
            // sygnal 2: Koniec pracy (SIGUSR2)
            struct msg_buf msg;
            msg.mtype = 1;
            msg.cmd = 2;
            msgsnd(msg_id, &msg, sizeof(int), 0);

            log_msg("DYSPOZYTOR: Wyslano polecenie STOP przez kolejke MSG");
            kill(cap_pid, SIGUSR2);
            break;
        }
    }

    log_msg("DYSPOZYTOR: Koniec pracy.");
    ipc_detach();
    return 0;
}
