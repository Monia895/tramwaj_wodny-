#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "common.h"
#include "ipc.h"
#include "log.h"
#include <sys/wait.h>
#include <sys/stat.h>

#define MAX_ACTIVE_PASSENGERS 100


volatile sig_atomic_t keep_running = 1;

// handler sprzatajacy
void cleanup_handler(int sig) {
     (void)sig;
    keep_running = 0;
}

void perform_cleanup(pid_t cap, pid_t disp) {
    printf("\nSYSTEM: Sprzatanie...\n");

    ipc_cleanup();
    log_close_parent();

    if (cap > 0) kill(cap, SIGKILL);
    if (disp > 0) kill(disp, SIGKILL);
    signal(SIGTERM, SIG_IGN);
    kill(0, SIGKILL);
}

int main(int argc, char *argv[]) {

    (void)argc;
    (void)argv;

    int N, M, K, T1, T2, R;

    // rejestracja sygnalow
    struct sigaction sa;
    sa.sa_handler = cleanup_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);

    // pobieranie i walidacja parametrow
    printf("Podaj parametry (N M K T1 T2 R): ");
    if (scanf("%d %d %d %d %d %d", &N, &M, &K, &T1, &T2, &R) != 6) {
        fprintf(stderr, "Blad: Niepoprawne dane wejsciowe.\n");
        return 1;
    }

    // Walidacja danych

    if (N > 500) { fprintf(stderr, "Za duze N. Max 500.\n"); return 1; }

    if (N <= 0 || M < 0 || K <= 0 || T1 <= 0 || T2 <= 0 || R <= 0) {
        fprintf(stderr, "Blad: Parametry musza byc dodatnie (M >= 0).\n");
        return 1;
    }
    if (M >= N) {
        fprintf(stderr, "Blad: Liczba rowerow (M) musi byc mniejsza od pojemnosci (N).\n");
        return 1;
    }
    if (K >= N) {
        fprintf(stderr, "Blad: Mostek (K) nie moze byc wiekszy od pojemnosci statku (N).\n");
        return 1;
    }
    if (K > MAX_K) {
        fprintf(stderr, "Blad: Maksymalna obslugiwana pojemnosc mostka to %d.\n", MAX_K);
        return 1;
    }

    // inicjalizacja loggera (tworzy FIFO)
    log_init_parent();

    // inicjalizacja IPC
    ipc_init_all(N, M, K, T1, T2, R);

    log_msg("SYSTEM: Start symulacji. N=%d, M=%d, K=%d", N, M, K);

    // uruchomienie kapitana
    pid_t pid_cap = fork();
    if (pid_cap == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        execl("./build/captain", "captain", NULL);
        perror("execl captain");
        exit(1);
    }

    // uruchomienie dyspozytora
    pid_t pid_disp = fork();
    if (pid_disp == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        execl("./build/dispatcher", "dispatcher", NULL);
        perror("execl dispatcher");
        exit(1);
    }


    int passengers_created = 0;
    int active_passengers = 0;

    while (keep_running) {
        sem_lock(SEM_MUTEX);
        if (state->ship_state == FINISHED) {
            sem_unlock(SEM_MUTEX);
            break;
        }
        sem_unlock(SEM_MUTEX);

        int status;
        pid_t ended_pid;
        while ((ended_pid = waitpid(-1, &status, WNOHANG)) > 0) {
            if (ended_pid == pid_cap || ended_pid == pid_disp) keep_running = 0;
            else active_passengers--;
        }

        if (passengers_created < N * R * 10 && active_passengers < MAX_ACTIVE_PASSENGERS) {
            pid_t pid = fork();
             if (pid == 0) {
                 signal(SIGINT, SIG_DFL);
                 signal(SIGTSTP, SIG_DFL);
                 signal(SIGTERM, SIG_DFL);
                 execl("./build/passenger", "passenger", NULL);
                 exit(1);
             } else if (pid > 0) {
                 passengers_created++;
                 active_passengers++;
             }
        } else {
           ended_pid = waitpid(-1, &status, 0);
            if (ended_pid > 0) {
                 if (ended_pid == pid_cap || ended_pid == pid_disp) keep_running = 0;
                 else active_passengers--;
            }
        }
    }

    if (keep_running && pid_cap > 0) {
        waitpid(pid_cap, NULL, 0);
    }

    perform_cleanup(pid_cap, pid_disp);

    return 0;
}
