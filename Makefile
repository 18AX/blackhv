BUILD_DIR=build
SRC_DIR=src

CC=gcc

SDL_CFLAGS=`sdl2-config --cflags`
SDL_LDFLAGS=`sdl2-config --libs`

CFLAGS+=-g3 -Wall -Werror -Wextra -pedantic $(SDL_CFLAGS) -Iinclude/
LFLAGS=-lasan -L./$(BUILD_DIR) -lblackhv $(SDL_LDFLAGS)

TARGET=$(BUILD_DIR)/libblackhv.a

.PHONY: all clean

OBJECTS=vm.o \
		linked_list.o \
		io.o \
		queue.o \
		serial.o \
		mmio.o \
		memory.o \
		screen.o \

all: $(TARGET)

$(TARGET): $(addprefix $(BUILD_DIR)/, $(OBJECTS))
	ar -rcs $(TARGET) $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@

tests: $(TARGET) $(BUILD_DIR)/tests.o
	$(CC) $(BUILD_DIR)/tests.o -o $(BUILD_DIR)/tests $(LFLAGS) -lcriterion
	./$(BUILD_DIR)/tests --verbose

clean:
	$(RM) -rf $(BUILD_DIR)
