CC = gcc
CFLAGS = -Wall -Wextra -I./src
LDFLAGS = -pthread -lrt

SRC_DIR = src
OBJ_DIR = obj

# List all .c files automatically
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Convert src/*.c → obj/*.o
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

TARGET = server
TARGET_TEST = test_concurrency

N ?= 10000

.PHONY: all clean

all: $(TARGET)

# MAKE and RUN with HELLGRIND
hellgrind: $(TARGET)
	valgrind --tool=helgrind --log-file=helgrind.log ./$(TARGET)

# MAKE and RUN with VALGRIND
valgrind: $(TARGET)
	valgrind --leak-check=full ./$(TARGET)

# MAKE and RUN
run: $(TARGET)
	./$(TARGET)

# Build and RUN OPTIMIZED
release: CFLAGS += -O3
release: clean all
	./$(TARGET)

# Build and run concurrency test
.PHONY: test
test: $(TARGET) $(TARGET_TEST)
	@echo "=== Starting server in background ==="
	@./$(TARGET) > /dev/null 2>&1 &
	@sleep 1 # Give server a moment to start
	@echo "=== Running concurrency test with $(N) connections ==="
	@./$(TARGET_TEST) $(N)
	@echo "=== Shutting down server ==="
	@pkill -f './$(TARGET)'

# Link final program
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile each .c → .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Create obj directory if missing
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(TARGET) $(TARGET_TEST)

# Build concurrency test program
$(TARGET_TEST): tests/TestConcurrency.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)