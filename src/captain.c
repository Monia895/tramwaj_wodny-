#include "common.h"
#include "log.h"
#include "ipc.h"

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    ipc_init();

    log_event("KAPITAN: Gotowy do pracy");

    printf("KAPITAN: Gotowy do pracy\n");
    return 0;
} 
