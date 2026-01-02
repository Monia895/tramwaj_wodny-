#include "common.h"
#include "ipc.h"
#include "log.h"

#include <sys/wait.h>

/*definicje zmiennych globalnych */
int N, M, K, T1, T2, R;

int shm_id;
int sem_id;
shared_state_t *state;

static void read_and_valide_input(void) {
    printf("Podaj parametry symulacji:\n");

    printf("N (maksymalna liczba pasazerow): ");
    if (scanf("%d", &N) != 1) {
        perror("scanf N");
        exit(EXIT_FAILURE);
    }

    printf("M (maksymalna liczba rowerow): ");
    if (scanf("%d", &M) != 1) {
        perror("scanf M");
        exit(EXIT_FAILURE);
    }

    printf("K (pojemnosc mostku): ");
    if (scanf("%d", &K) != 1) {
        perror("scanf K");
        exit(EXIT_FAILURE);
    }

    printf("T1 (czas do wyplyniecia): ");
    if (scanf("%d", &T1) != 1) {
        perror("scanf T1");
        exit(EXIT_FAILURE);
    }

    printf("T2 (czas rejsu): ");
    if (scanf("%d", &T2) != 1) {
        perror("scanf T2");
        exit(EXIT_FAILURE);
    }

    printf("R (maksymalna liczba rejsow): ");
    if (scanf("%d", &R) != 1) {
        perror("scanf R");
        exit(EXIT_FAILURE);
    }

    /* walidacja */
    if (N <= 0) {
       fprintf(stderr, "Blad: N musi byc > 0\n");
       exit(EXIT_FAILURE);
    }

    if (M < 0 || M >= N) {
       fprintf(stderr, "Blad: M musi byc >= 0 i < N\n");
       exit(EXIT_FAILURE);
    }

    if (K <= 0 || K >= N) {
       fprintf(stderr, "Blad: K musi byc > 0 i < N\n");
       exit(EXIT_FAILURE);
    }

    if (T1 <= 0 || T2 <= 0) {
       fprintf(stderr, "Blad: T1 i T2 musza byc > 0\n");
       exit(EXIT_FAILURE);
    }

    if (R <= 0) {
       fprintf(stderr, "Blad: R musi byc > 0\n");
       exit(EXIT_FAILURE);
    }
    printf("Parametry poprawne. Start symulacji.\n\n");
}

int main(void) {
    pid_t pid;

    /* wejscie i walidacja */
    read_and_valide_input();

    /* inicjalizacja ipc */
    ipc_init();

    /* inicjalizacja stanu wspolnego */
    state->passengers_on_ship = 0;
    state->bikes_on_ship = 0;
    state->passengers_on_bridge = 0;

    log_event("MAIN: Start symulacji tramwaju wodnego");
    printf("MAIN: Start symulacji tramwaju wodnego\n");

    /* uruchomienie kapitana */
    pid = fork();
    if (pid == -1) {
        perror("fork captain");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        execl("./build/captain", "captain", NULL);
        perror("execl captain");
        exit(EXIT_FAILURE);
   }

   /* uruchomienie dyspozytora */
   pid = fork();
   if (pid == -1) {
       perror("fork dispatcher");
       exit(EXIT_FAILURE);
   }
   if (pid == 0) {
       execl("./build/dispatcher", "dispatcher", NULL);
       perror("execl dispatcher");
       exit(EXIT_FAILURE);
   }

   for (int i = 0; i < 3; i++) {
       pid = fork();
       if (pid == -1) {
           perror("fork passenger");
           exit(EXIT_FAILURE);
       }
       if (pid == 0) {
          execl("./build/passenger", "passenger", NULL);
          perror("execl passenger");
          exit(EXIT_FAILURE);
       }
    }

    /* czekanie na zakonczenie procesow potomnych */
    while (wait(NULL) > 0);

    log_event("MAIN: Koniec symulacji");
    printf("MAIN: Koniec symulacji\n");

    /* sprzatanie ipc */
    ipc_cleanup();

    return 0;
}
