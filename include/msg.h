#ifndef MSG_H
#define MSG_H

struct msg_buf {
    long mtype;
    int cmd; // 1 = EARLY_DEPARTURE, 2 = STOP
};

#endif
