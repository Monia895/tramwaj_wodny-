#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "ipc.h"
#include "log.h"
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
    const int MAX_ATTEMPTS = 500;

    while (attempts < MAX_ATTEMPTS) {
        attempts++;

       int sleep_time = 5 + (rand() % 10);

        for (int i = 0; i < sleep_time * 10; i++) {
            sem_lock(SEM_MUTEX);
            if (state->ship_state == FINISHED) {
                sem_unlock(SEM_MUTEX);
                goto end_work;
            }
            sem_unlock(SEM_MUTEX);
  //          usleep(100000);
        }

        sem_lock(SEM_MUTEX);
        if (state->ship_state == FINISHED) {
            sem_unlock(SEM_MUTEX);
            break;
        }
        pid_t cap_pid = state->captain_pid;
        ship_state_t st = state->ship_state;
        sem_unlock(SEM_MUTEX);

        int action = rand() % 1000;

        if (cap_pid > 0 && action < 200 && st == LOADING) {
            // sygnal 1: Odplywaj wczesniej (SIGUSR1)
            log_msg("DYSPOZYTOR: Wysylam SIGUSR1 do kapitana PID=%d", cap_pid);
            kill(cap_pid, SIGUSR1);
        }
        else if (cap_pid > 0 && action >= 995) {
            // sygnal 2: Koniec pracy (SIGUSR2)

            log_msg("DYSPOZYTOR: Wysylam SIGUSR2 (koniec pracy) do kapitana PID=%d", cap_pid);
            kill(cap_pid, SIGUSR2);
            break;
        }
    }

end_work:
    log_msg("DYSPOZYTOR: Koniec pracy.");
    ipc_detach();
    return 0;
}
