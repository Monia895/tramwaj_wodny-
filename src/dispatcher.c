#include "common.h"
#include "log.h"
#include "ipc.h"

int main(int argc, char *argv[]) {
   (void)argc;
   (void)argv;

   ipc_init();

   log_event("DYSPOZYTOR: Gotowy do pracy");

   printf("DYSPOZYTOR: Monitoruje rejsy\n");
   return 0;
}

