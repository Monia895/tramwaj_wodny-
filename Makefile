CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
CFLAGS += -pthread
BUILD = build

TARGETS = main captain dispatcher passenger

all: $(TARGETS)

$(BUILD):
	mkdir -p $(BUILD)

main: $(BUILD) src/main.c src/log.c src/ipc.c src/ipc_globals.c
	$(CC) $(CFLAGS) src/main.c src/log.c src/ipc.c src/ipc_globals.c -o $(BUILD)/main

captain: $(BUILD) src/captain.c src/log.c src/ipc.c src/ipc_globals.c
	$(CC) $(CFLAGS) src/captain.c src/log.c src/ipc.c src/ipc_globals.c -o $(BUILD)/captain

dispatcher: $(BUILD) src/dispatcher.c src/log.c src/ipc.c src/ipc_globals.c
	$(CC) $(CFLAGS) src/dispatcher.c src/log.c src/ipc.c src/ipc_globals.c -o $(BUILD)/dispatcher

passenger: $(BUILD) src/passenger.c src/log.c src/ipc.c src/ipc_globals.c
	$(CC) $(CFLAGS) src/passenger.c src/log.c src/ipc.c src/ipc_globals.c -o $(BUILD)/passenger

clean:
	rm -rf $(BUILD)

