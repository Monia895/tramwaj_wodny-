#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>

typedef enum {
LOADING,
SAILING,
UNLOADING,
STOPPED
} ship_state_t;

#endif
