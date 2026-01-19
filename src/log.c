#include "log.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

int log_file_fd = -1;

void log_init_parent(void) {
    log_file_fd = open("simulation.log", O_WRONLY | O_CREAT | O_APPEND | O_TRUNC, 0644);
    if (log_file_fd == -1) {
        perror("Nie udalo sie otworzyc pliku logow");
    }
}

void log_close_parent(void) {
    if (log_file_fd != -1) close(log_file_fd);
}

void log_msg(const char *fmt, ...) {
    if (log_file_fd == -1) {
        log_file_fd = open("simulation.log", O_WRONLY | O_CREAT | O_APPEND, 0666);
        if (log_file_fd == -1) return;
    }

    char buf[512];
    va_list args;
    
    // Formatowanie komunikatu
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    // Dodajemy nowa linie
    size_t len = strlen(buf);
    if (len < sizeof(buf) - 2) {
            buf[len] = '\n';
            buf[len+1] = '\0';
            len++;
    }

    write(log_file_fd, buf, len);
    if (strstr(buf, "SYSTEM") || strstr(buf, "KAPITAN") || strstr(buf, "DYSPOZYTOR")) {
        write(STDOUT_FILENO, buf, len);
    }
}
