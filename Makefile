BUILD_DIR=build
SRC_DIR=src

CC=gcc
CFLAGS+=-g3 -Wall -Werror -Wextra -pedantic -Iinclude/
LFLAGS=-lasan -L./$(BUILD_DIR) -lblackhv

TARGET=$(BUILD_DIR)/libblackhv.a

.PHONY: all clean

OBJECTS=vm.o \
		linked_list.o \
		io.o \
		queue.o \
		serial.o \

all: $(TARGET)

$(TARGET): $(addprefix $(BUILD_DIR)/, $(OBJECTS))
	ar -rcs $(TARGET) $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@

tests: $(TARGET) $(BUILD_DIR)/tests.o
	$(CC) $(BUILD_DIR)/tests.o -o $(BUILD_DIR)/tests $(LFLAGS) -lcriterion
	./$(BUILD_DIR)/tests --verbose

example: $(TARGET) $(BUILD_DIR)/main.o
	$(CC) $(BUILD_DIR)/main.o -o $(BUILD_DIR)/example $(LFLAGS)

clean:
	$(RM) -rf $(BUILD_DIR)
