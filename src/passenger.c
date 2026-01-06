#define _POSIX_C_SOURCE 200809L
#include "common.h"
#include "log.h"
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(void) {
    srand(getpid() ^ time(NULL));

    key_t key = ftok(".", 'S');
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    shm_id = shmget(key, sizeof(shared_state_t), 0);
    if (shm_id == -1) {
        perror("shmget");
        exit(1);
    }

    sem_id = semget(key, SEM_COUNT, 0);
    if (sem_id == -1) {
        perror("semget");
        exit(1);
    }

    state = shmat(shm_id, NULL, 0);
    if (state == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    sem_lock(sem_id, SEM_STATE);
    port_t port = state->current_port;
    sem_unlock(sem_id, SEM_STATE);

    log_event("PASAZER [%d]: Czeka w porcie %s",
        getpid(),
        port == KRAKOW ? "KRAKOW" : "TYNIEC");

    /* losowanie roweru */
    int has_bike = (rand() % 10 < 3);
    int bridge_weight = has_bike ? 2 : 1;
    pid_t pid = getpid();


    log_event("PASAZER [%d]: Przybyl (rower: %s)",
              pid, has_bike ? "TAK" : "NIE");

    int boarded = 0;

    while (!boarded) {

        while (1) {
            sem_lock(sem_id, SEM_STATE);

            if (state->ship_state == LOADING) {
                sem_unlock(sem_id, SEM_STATE);
                break;
            }

            if (state->ship_state == FINISHED) {
                sem_unlock(sem_id, SEM_STATE);
                log_event("PASAZER [%d]: Koniec rejsow - rezygnuje", pid);
                shmdt(state);
                return 0;
            }

            sem_unlock(sem_id, SEM_STATE);
            usleep(100000);
        }

        sem_lock(sem_id, SEM_SHIP);

        if (state->passengers_on_ship >= N ||
            (has_bike && state->bikes_on_ship >= M)) {

            sem_unlock(sem_id, SEM_SHIP);
            usleep(300000);
            continue;
        }

        sem_unlock(sem_id, SEM_SHIP);

        sem_lock(sem_id, SEM_STATE);
        if (state->bridge_dir != TO_SHIP) {
            sem_unlock(sem_id, SEM_STATE);
            usleep(200000);
            continue;
        }
        sem_unlock(sem_id, SEM_STATE);

        struct sembuf bridge_down = {
            .sem_num = SEM_BRIDGE,
            .sem_op = -bridge_weight,
            .sem_flg = 0
        };

        semop(sem_id, &bridge_down, 1);

        sem_lock(sem_id, SEM_STATE);
        state->passengers_on_bridge += bridge_weight;
        sem_unlock(sem_id,SEM_STATE);

        log_event("PASAZER [%d]: Na mostku (waga %d)", pid, bridge_weight);

        sem_lock(sem_id, SEM_STATE);
        if (state->boarding_closed) {

            state->passengers_on_bridge -= bridge_weight;
            sem_unlock(sem_id, SEM_STATE);

            struct sembuf bridge_up = {
                .sem_num = SEM_BRIDGE,
                .sem_op = bridge_weight,
                .sem_flg = 0
            };
            semop(sem_id, &bridge_up, 1);

            log_event("PASAZER [%d]: Schodzi z mostka (zamkniete wejscie)", pid);
            shmdt(state);
            return 0;
        }
        sem_unlock(sem_id, SEM_STATE);

        sem_lock(sem_id, SEM_STATE);
        if (state->ship_state == FINISHED) {

            state->passengers_on_bridge -= bridge_weight;
            sem_unlock(sem_id, SEM_STATE);

        sem_lock(sem_id, SEM_STATE);
        if (state->bridge_dir != FROM_SHIP) {
            sem_unlock(sem_id, SEM_STATE);
            usleep(200000);
            continue;
        }
        sem_unlock(sem_id, SEM_STATE);

            struct sembuf bridge_up = {
                .sem_num = SEM_BRIDGE,
                .sem_op = bridge_weight,
                .sem_flg = 0
            };
            semop(sem_id, &bridge_up, 1);

            log_event("PASAZER [%d]: Schodzi z mostka (koniec pracy statku)", pid);
            shmdt(state);
            return 0;
        }
        sem_unlock(sem_id, SEM_STATE);


        sem_lock(sem_id, SEM_STATE);
        sem_lock(sem_id, SEM_SHIP);

        int can_board =
            (state->ship_state == LOADING) &&
            (state->passengers_on_ship < N) &&
            (!has_bike || state->bikes_on_ship < M);

        if (can_board) {
            state->passengers_on_ship++;
            if (has_bike) state->bikes_on_ship++;

            state->passengers_on_bridge -= bridge_weight;

            sem_unlock(sem_id, SEM_SHIP);
            sem_unlock(sem_id, SEM_STATE);

            struct sembuf bridge_up = {
                .sem_num = SEM_BRIDGE,
                .sem_op = bridge_weight,
                .sem_flg = 0
            };

            log_event(
                "PASAZER [%d]: Wszedl na statek (L:%d%d R:%d%d)",
                pid,
                state->passengers_on_ship, N,
                state->bikes_on_ship, M
            );

            int fd = open(TRAM_FIFO, O_WRONLY | O_NONBLOCK);
            if (fd != -1) {
                char msg[128];
                snprintf(msg, sizeof(msg),
                         "FIFO: PASAZER %d wszedl na statek\n", pid);
                write(fd, msg, strlen(msg));
                close(fd);
            }

            boarded = 1;
        } else {
            state->passengers_on_bridge -= bridge_weight;

            sem_unlock(sem_id, SEM_SHIP);
            sem_unlock(sem_id, SEM_STATE);

            struct sembuf bridge_up = {
                .sem_num = SEM_BRIDGE,
                .sem_op = bridge_weight,
                .sem_flg = 0
            };
            semop(sem_id, &bridge_up, 1);

            log_event("PASAZER [%d]: Zszedl z mostku (brak miejsca)", pid);
            usleep(300000);
        }
    }

    while (1) {
        sem_lock(sem_id, SEM_STATE);
        if (state->ship_state == UNLOADING) {
            sem_unlock(sem_id, SEM_STATE);
            break;
        }
        sem_unlock(sem_id, SEM_STATE);
        sleep(1);
    }

    log_event("PASAZER [%d]: Opuszcza statek", pid);
    shmdt(state);
    return 0;
}

