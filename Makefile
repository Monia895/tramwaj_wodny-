CC = gcc
CFLAGS = -Wall -Wextra -pthread -Iinclude
BUILD_DIR = build
SRC_DIR = src

TARGETS = main captain dispatcher passenger

all: $(BUILD_DIR) $(addprefix $(BUILD_DIR)/, $(TARGETS))

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Reguly kompilacji
$(BUILD_DIR)/main: $(SRC_DIR)/main.c $(SRC_DIR)/ipc.c $(SRC_DIR)/log.c
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/captain: $(SRC_DIR)/captain.c $(SRC_DIR)/ipc.c $(SRC_DIR)/log.c
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/dispatcher: $(SRC_DIR)/dispatcher.c $(SRC_DIR)/ipc.c $(SRC_DIR)/log.c
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/passenger: $(SRC_DIR)/passenger.c $(SRC_DIR)/ipc.c $(SRC_DIR)/log.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(BUILD_DIR) simulation.log /tmp/tram_log_fifo

.PHONY: all clean
