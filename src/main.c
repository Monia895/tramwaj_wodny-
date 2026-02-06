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

volatile sig_atomic_t keep_running = 1;
int active_passengers = 0;
pid_t pid_cap = 0;
pid_t pid_disp = 0;
int finished_passengers = 0;

pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

void perform_cleanup(void);

// handler sprzatajacy
void cleanup_handler(int sig) {
     (void)sig;
     if (keep_running == 0) {
         const char *msg = "\nSYSTEM: DRUGIE Ctrl+C -> WYMUSZAM SMIERC (SIGKILL)!\n";
         write(STDOUT_FILENO, msg, 50);

         kill(0, SIGKILL);
         _exit(1);
     }

     keep_running = 0;
     const char *msg = "\nSYSTEM: Otrzymano Ctrl+C. Koncze... (Nacisnij ponownie by zabic natychmiast)\n";
     write(STDOUT_FILENO, msg, 78);

}

// watek czyszczacy
void *zombie_cleaner_func(void *arg) {
    (void)arg;
    while (keep_running || active_passengers > 0) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);

        if (pid > 0) {
            if (pid == pid_cap) {
                keep_running = 0;
            }
            else if (pid != pid_disp) {
                pthread_mutex_lock(&count_mutex);
                if (active_passengers > 0) active_passengers--;
                finished_passengers++;
                pthread_mutex_unlock(&count_mutex);
            }
        } else {
//            usleep(50000);
        }
    }
    return NULL;
}

void perform_cleanup(void) {

    ipc_cleanup();
    log_close_parent();

    kill(0, SIGKILL);
}

int main(int argc, char *argv[]) {

    (void)argc;
    (void)argv;

    setbuf(stdout, NULL);

    int N, M, K, T1, T2, R;

    // rejestracja sygnalow


    struct sigaction sa;
    sa.sa_handler = cleanup_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

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

    int max_capacity = (N * R) / 2;

    int target_passengers = (int)(max_capacity * 0.8);

    if (target_passengers < 1) target_passengers = 1;

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


    int generated = 0;

    while (keep_running && generated < target_passengers) {
        sem_lock(SEM_MUTEX);
        if (state->ship_state == FINISHED) {
            sem_unlock(SEM_MUTEX);
            keep_running = 0;
            break;
        }
        sem_unlock(SEM_MUTEX);

        int current;
        pthread_mutex_lock(&count_mutex);
        current = active_passengers;
        pthread_mutex_unlock(&count_mutex);

        if (current < 800) {
            pid_t pid = fork();
             if (pid == 0) {
                 execl("./build/passenger", "passenger", NULL);
                 exit(1);
             } else if (pid > 0) {
                 generated++;
                 pthread_mutex_lock(&count_mutex);
                 active_passengers++;
                 pthread_mutex_unlock(&count_mutex);
             }

             if (generated % 100 == 0) {
                 pthread_mutex_lock(&count_mutex);
                 int finished = finished_passengers;
                 int active = active_passengers;
                 pthread_mutex_unlock(&count_mutex);

                 printf("[LIVE] Gen: %d | Akt: %d | Fin: %d\n", generated, active, finished);
                 fflush(stdout);
            }

        } else {
  //          usleep(10000);
        }
    //    usleep(2000);
    }

    printf("SYSTEM: Wygenerowano %d pasazerow. Czekam na ukonczenie kursow...\n", generated);

    if (pid_disp > 0) {
        kill(pid_disp, SIGKILL);
    }

    int timeout_counter = 0;
    while (active_passengers > 0) {
        if (!keep_running) {
            timeout_counter++;

            if (timeout_counter > 5) {
                printf("SYSTEM: Pasazerowie utkneli. Zarzadzam przymusowy koniec.\n");
                break;
            }
        }

      //  sleep(1);

    pthread_mutex_lock(&count_mutex);
    int rem = active_passengers;
    int fin = finished_passengers;
    pthread_mutex_unlock(&count_mutex);

    if (rem > 0) {
        printf("[LIVE] Czekam... Aktywni: %d | Juz obsłużeni: %d\n", rem, fin);
        fflush(stdout);
    }

    }

    printf("\n==PODSUMOWANIE==\n");
    fflush(stdout);
    printf("Planowano: %d\n", target_passengers);
    fflush(stdout);
    printf("Obsluzono:  %d\n", finished_passengers);
    fflush(stdout);

    if (finished_passengers >= target_passengers - 5) {
        printf("WYNIK: Wszyscy pasazerowie obsluzeni.\n");
    } else {
        printf("WYNIK: Brakuje %d pasazerow.\n", target_passengers - finished_passengers);
    }
    fflush(stdout);

    keep_running = 0;
    perform_cleanup();

    return 0;
}
