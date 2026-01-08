#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "log.h"
#include "common.h"
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>

static int fifo_fd_read = -1;
static pthread_t log_thread;
static volatile int log_running = 1;

// watek zapisujacy logi z FIFO do pliku/konsoli
void *logger_thread_func(void *arg) {
    (void)arg;
    char buf[512];
    FILE *log_file = fopen("simulation.log", "w");
    if (!log_file) { perror("fopen log"); return NULL; }

    while (log_running) {
        ssize_t n = read(fifo_fd_read, buf, sizeof(buf)-1);
        if (n > 0) {
            buf[n] = '\0';
            fprintf(log_file, "%s", buf);
            fprintf(stdout, "%s", buf);
            fflush(log_file);
            fflush(stdout);
        } else if (n == 0) {
            usleep(10000);
        }
    }
    fclose(log_file);
    return NULL;
}

// inicjalizacja logowania w procesie
void log_init_parent(void) {
    mkfifo(FIFO_NAME, 0600);
    fifo_fd_read = open(FIFO_NAME, O_RDONLY | O_NONBLOCK);
    if (fifo_fd_read == -1) { perror("open fifo read"); return; }

    pthread_create(&log_thread, NULL, logger_thread_func, NULL);
}

// zamkniecie logowania i watku
void log_close_parent(void) {
    log_running = 0;
    pthread_join(log_thread, NULL);
    close(fifo_fd_read);
    unlink(FIFO_NAME);
}

// funkcja wysylajaca log do FIFO
void log_msg(const char *fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    strcat(buf, "\n");

    int fd = open(FIFO_NAME, O_WRONLY | O_NONBLOCK);
    if (fd != -1) {
        write(fd, buf, strlen(buf));
        close(fd);
    }
}

