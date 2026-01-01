#include "common.h"
#include <sys/wait.h>

int main() {
    pid_t pid;

    pid = fork();
    if (pid == 0) { 
        execl("./build/captain", "captain", NULL);
        perror("exec captain");
        exit(1);
   }

   pid = fork();
   if (pid == 0) {
      execl("./build/dispatcher", "dispatcher", NULL);
      perror("exec dispatcher");
      exit(1);
   }

  pid = fork();
  if (pid == 0) {
     execl("./build/passenger", "passenger", NULL);
     perror("exec passenger");
     exit(1);
  }

  while (wait(NULL) > 0);

  return 0;
}
