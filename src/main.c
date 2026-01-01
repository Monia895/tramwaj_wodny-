#include "common.h"
#include <sys/wait.h>

int main(void) {
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
