#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "common.h"
#include "ipc.h"
#include "log.h"
#include <sys/wait.h>
#include <sys/stat.h>


void cleanup_handler(int sig) {
    (void)sig;
    printf("\nPrzechwycono sygnal. Sprzatanie...\n");
    ipc_cleanup();
    log_close_parent();
    exit(0);
}

int main(int argc, char *argv[]) {

    (void)argc;
    (void)argv;

    int N, M, K, T1, T2, R;

    signal(SIGINT, cleanup_handler);

    printf("Podaj parametry (N M K T1 T2 R): ");
    if (scanf("%d %d %d %d %d %d", &N, &M, &K, &T1, &T2, &R) != 6) {
        fprintf(stderr, "Blad: Niepoprawne dane wejsciowe.\n");
        return 1;
    }

    // Walidacja danych
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
        execl("./build/captain", "captain", NULL);
        perror("execl captain");
        exit(1);
    }

    // uruchomienie dyspozytora
    pid_t pid_disp = fork();
    if (pid_disp == 0) {
        execl("./build/dispatcher", "dispatcher", NULL);
        perror("execl dispatcher");
        exit(1);
    }

    // generowanie pasazerow
    int passengers_created = 0;
    while (1) {
        sem_lock(SEM_MUTEX);
        if (state->ship_state == FINISHED) {
            sem_unlock(SEM_MUTEX);
            break;
        }
        sem_unlock(SEM_MUTEX);

        if (passengers_created < R * N * 3) {
            pid_t pid_pass = fork();
            if (pid_pass == 0) {
                execl("./build/passenger", "passenger", NULL);
                perror("execl passenger");
                exit(1);
            }
            passengers_created++;
        }

        usleep(200000);
        waitpid(-1, NULL, WNOHANG);
    }

    waitpid(pid_cap, NULL, 0);
    kill(pid_disp, SIGKILL);

    log_msg("SYSTEM: Koniec symulacji.");

    // sprzatanie
    ipc_cleanup();
    log_close_parent();

    return 0;
}
