CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
BUILD = build

TARGETS = main captain dispatcher passenger

all: $(TARGETS)

$(BUILD):
	mkdir -p $(BUILD)

main: $(BUILD) src/main.c
	$(CC) $(CFLAGS) src/main.c -o $(BUILD)/main

captain: $(BUILD) src/captain.c
	$(CC) $(CFLAGS) src/captain.c -o $(BUILD)/captain

dispatcher: $(BUILD) src/dispatcher.c
	$(CC) $(CFLAGS) src/dispatcher.c -o $(BUILD)/dispatcher

passenger: $(BUILD) src/passenger.c
	$(CC) $(CFLAGS) src/passenger.c -o $(BUILD)/passenger

clen:
	rm -rf $(BUILD)

