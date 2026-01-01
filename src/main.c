#include "common.h"
#include <sys/wait.h>

int N, M, K, T1, T2, R;

int main(void) {

    printf("Podaj parametry symulacji:\n");

    printf("N (maksymalna liczba pasazerow): ");
    if (scanf("%d", &N) != 1) {
        perror("Blad wczytywania N");
        exit(1);
    }

    printf("M (maksymalna liczba rowerow): ");
    if (scanf("%d", &M) != 1) {
        perror("Blad wczytywania M");
        exit(1);
    }

    printf("K (pojemnosc mostku): ");
    if (scanf("%d", &K) != 1) {
        perror("Blad wczytywania K");
        exit(1);
    }

    printf("T1 (czas do wyplyniecia): ");
    if (scanf("%d", &T1) != 1) {
        perror("Blad wczytywania T1");
        exit(1);
    }

    printf("T2 (czas rejsu): ");
    if (scanf("%d", &T2) != 1) {
        perror("Blad wczytywania T2");
        exit(1);
    }

    printf("R (maksymalna liczba rejsow): ");
    if (scanf("%d", &R) != 1) {
        perror("Blad wczytywania R");
        exit(1);
    }

    if (N <= 0) {
       fprintf(stderr, "Blad: N musi byc > 0\n");
       exit(1);
    }

    if (M < 0 || M >= N) {
       fprintf(stderr, "Blad: M musi byc >= 0 i < N\n");
       exit(1);
    }

    if (K <= 0 || K >= N) {
       fprintf(stderr, "Blad: K musi byc > 0 i < N\n");
       exit(1);
    }

    if (T1 <= 0 || T2 <= 0) {
       fprintf(stderr, "Blad: T1 i T2 musza byc > 0\n");
       exit(1);
    }

    if (R <= 0) {
       fprintf(stderr, "Blad: R musi byc > 0\n");
       exit(1);
    }

    printf("Parametry poprawne. Start symulacji.\n\n");

    pid_t pid;

    printf("MAIN: Start symulacji tramwaju wodnego\n");

    pid = fork();
    if (pid == 0) { 
        execl("./build/captain", "captain", NULL);
        perror("execl captain");
        exit(1);
   }

   pid = fork();
   if (pid == 0) {
      execl("./build/dispatcher", "dispatcher", NULL);
      perror("execl dispatcher");
      exit(1);
   }
for (int i = 0; i < 3; i++) {
  pid = fork();
  if (pid == 0) {
     execl("./build/passenger", "passenger", NULL);
     perror("execl passenger");
     exit(1);
  }
}

  while (wait(NULL) > 0);
  
  printf("MAIN: Koniec symulacji\n");
  return 0;
}
