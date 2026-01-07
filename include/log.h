#ifndef LOG_H
#define LOG_H

#include <pthread.h>

void log_init_parent(void);
void log_close_parent(void);
void log_msg(const char *fmt, ...);

#endif
