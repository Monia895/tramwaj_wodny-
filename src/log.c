#include "log.h"
#include "common.h"

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/sem.h>
#include <errno.h>
#include <pthread.h>

#define LOG_FILE "simulation.log"
#define MAX_LOG_LEN 256

extern pthread_mutex_t log_mutex;
extern pthread_cond_t  log_cond;

extern char log_buffer[256];
extern int log_ready;

void log_event(const char *fmt, ...) {
    char buffer[256];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    pthread_mutex_lock(&log_mutex);

    strncpy(log_buffer, buffer, sizeof(log_buffer));
    log_ready = 1;

    pthread_cond_signal(&log_cond);
    pthread_mutex_unlock(&log_mutex);
}

