#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>

extern int N;
extern int M;
extern int K;
extern int T1;
extern int T2;
extern int R;

typedef enum {
LOADING,
SAILING,
UNLOADING,
STOPPED
} ship_state_t;

#endif
