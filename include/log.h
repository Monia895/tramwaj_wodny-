#ifndef LOG_H
#define LOG_H

// funkcje logowania
void log_init_parent(void);
void log_close_parent(void);
void log_msg(const char *fmt, ...);

#endif
