#define _POSIX_C_SOURCE 200809L
#include "ipc.h"
#include "log.h"
#include "msg.h"
#include <time.h>
#include <signal.h>
#include <sys/msg.h>
#include <unistd.h>

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

    while (1) {
        // losowe oczekiwanie
        alarm(rand() % 10 + 5);
        pause();

        sem_lock(SEM_MUTEX);
        pid_t cap_pid = state->captain_pid;
        ship_state_t st = state->ship_state;
        sem_unlock(SEM_MUTEX);

        if (st == FINISHED) break;

        int action = rand() % 10;

        if (action < 3 && st == LOADING) {
            // sygnal 1: Odplywaj wczesniej (SIGUSR1)
            log_msg("DYSPOZYTOR: Nakaz wczesniejszego wyplyniecia (SIGUSR1)");
            kill(cap_pid, SIGUSR1);
        }
        else if (action == 9) {
            // sygnal 2: Koniec pracy (SIGUSR2)
            struct msg_buf msg;
            msg.mtype = 1;
            msg.cmd = 2;
            msgsnd(msg_id, &msg, sizeof(int), 0);
            kill(cap_pid, SIGUSR2);
                 log_msg("DYSPOZYTOR: Wyslano polecenie STOP przez kolejke MSG");
            break;
        }
    }

    ipc_detach();
    return 0;
}
