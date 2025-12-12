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

.PHONY: all clean

all: $(TARGET)

# MAKE and RUN with HELLGRIND
hellgrind: $(TARGET)
	valgrind --tool=helgrind ./$(TARGET)

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
	rm -rf $(OBJ_DIR) $(TARGET)