#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define LOG_FILE "simulation.log"

/* synchronizacja watku */
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  log_cond  = PTHREAD_COND_INITIALIZER;

/* bufor logów */
static char log_buffer[256];
static int log_ready = 0;
static int logger_running = 1;

/* watek loggera */
void *logger_thread(void *arg) {
    FILE *f = fopen(LOG_FILE, "a");
    if (!f) {
        perror("fopen log");
        return NULL;
    }

    int fifo_fd = open(TRAM_FIFO, O_RDONLY | O_NONBLOCK);

    while (logger_running) {
    char fifo_buf[128];
    int n = read(fifo_fd, fifo_buf, sizeof(fifo_buf) - 1);

    if (n > 0) {
        fifo_buf[n] = '\0';
        fprintf(f, "%s", fifo_buf);
        fflush(f);
    }

    pthread_mutex_lock(&log_mutex);

        while (!log_ready && logger_running)
            pthread_cond_wait(&log_cond, &log_mutex);

        if (log_ready) {
            fprintf(f, "%s\n", log_buffer);
            fflush(f);
            log_ready = 0;
        }

        pthread_mutex_unlock(&log_mutex);
    }

    fclose(f);
    return NULL;
}
