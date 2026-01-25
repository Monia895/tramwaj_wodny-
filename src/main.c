#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "common.h"
#include "ipc.h"
#include "log.h"
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>

#define MAX_ACTIVE_PASSENGERS 10000

void perform_cleanup(void);

volatile sig_atomic_t keep_running = 1;
int active_passengers = 0;
pid_t pid_cap = 0;
pid_t pid_disp = 0;

pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

// watek czyszczacy
void *zombie_cleaner_func(void *arg) {
    (void)arg;
    int status;
    pid_t pid;

    while (1) {
        pid = waitpid(-1, &status, 0);

        if (pid > 0) {
            if (pid == pid_cap) {
                keep_running = 0;
                if (pid_disp > 0) kill(pid_disp, SIGKILL);
                kill(0, SIGKILL);
                return NULL;
            }
            else if (pid != pid_disp) {
                pthread_mutex_lock(&count_mutex);
                if (active_passengers > 0) active_passengers--;
                pthread_mutex_unlock(&count_mutex);
            }
        }
        else {
            if (errno == ECHILD) {
                return NULL;
            }
        }
    }
    return NULL;
}

// handler sprzatajacy
void cleanup_handler(int sig) {
     (void)sig;
     keep_running = 0;
     perform_cleanup();
}

void perform_cleanup(void) {
    printf("\nSYSTEM: Sprzatanie...\n");

    ipc_cleanup();
    log_close_parent();

    if (pid_cap > 0) kill(pid_cap, SIGKILL);
    if (pid_disp > 0) kill(pid_disp, SIGKILL);
    signal(SIGTERM, SIG_DFL);
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

    // pobieranie i walidacja parametrow
    printf("Podaj parametry (N M K T1 T2 R): ");
    if (scanf("%d %d %d %d %d %d", &N, &M, &K, &T1, &T2, &R) != 6) {
        fprintf(stderr, "Blad: Niepoprawne dane wejsciowe.\n");
        return 1;
    }

    // Walidacja danych

    if (N > 500) { fprintf(stderr, "Za duze N. Max 500.\n"); return 1; }

    if (N < 0 || M < 0 || K < 0 || T1 < 0 || T2 < 0 || R < 0) {
        fprintf(stderr, "Blad: Parametry musza byc dodatnie.\n");
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

    if (T1 <= 0) T1 = 1;
    if (T2 <= 0) T2 = 1;

    // inicjalizacja loggera (tworzy FIFO)
    log_init_parent();

    // inicjalizacja IPC
    ipc_init_all(N, M, K, T1, T2, R);

    log_msg("SYSTEM: Start symulacji. N=%d, M=%d, K=%d", N, M, K);

    // start watku czyszczacego
    pthread_t cleaner_thread;
    if (pthread_create(&cleaner_thread, NULL, zombie_cleaner_func, NULL) != 0) {
        perror("Thread error");
        return 1;
    }

    // uruchomienie kapitana
    pid_t pid_cap = fork();
    if (pid_cap == 0) {
        signal(SIGINT, SIG_DFL);
        execl("./build/captain", "captain", NULL);
        perror("execl captain");
        exit(1);
    }

    // uruchomienie dyspozytora
    pid_t pid_disp = fork();
    if (pid_disp == 0) {
        signal(SIGINT, SIG_DFL);
        execl("./build/dispatcher", "dispatcher", NULL);
        perror("execl dispatcher");
        exit(1);
    }


    int passengers_created = 0;

    while (keep_running) {
        sem_lock(SEM_MUTEX);
        if (state->ship_state == FINISHED) {
            sem_unlock(SEM_MUTEX);
            keep_running = 0;
            break;
        }
        sem_unlock(SEM_MUTEX);

        int current_active;
        pthread_mutex_lock(&count_mutex);
        current_active = active_passengers;
        pthread_mutex_unlock(&count_mutex);

        if (passengers_created < N * R * 10 && current_active < MAX_ACTIVE_PASSENGERS) {
            pid_t pid = fork();
             if (pid == 0) {
                 signal(SIGINT, SIG_DFL);
                 signal(SIGTERM, SIG_DFL);
                 execl("./build/passenger", "passenger", NULL);
                 exit(1);
             } else if (pid > 0) {
                 passengers_created++;
                 pthread_mutex_lock(&count_mutex);
                 active_passengers++;
                 pthread_mutex_unlock(&count_mutex);
             }
        } else {
           custom_sleep(1);
        }
        usleep(100000);
    }

    printf("\nSYSTEM: Czekam na zakonczenie watku sprzatajacego...\n");
    pthread_join(cleaner_thread, NULL);
    pthread_mutex_destroy(&count_mutex);

    perform_cleanup();
    printf("SYSTEM: Koniec.\n");

    return 0;
}
