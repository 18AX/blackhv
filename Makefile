CC=gcc
CFLAGS+=-g3 -Wall -Werror -Wextra -pedantic -Iinclude/ -fsanitize=address
LFLAGS= -lasan

BUILD_DIR=build
SRC_DIR=src

TARGET=$(BUILD_DIR)/blackhv

.PHONY: all clean

OBJECTS=main.o \
		vm.o \
		linked_list.o \
		io.o \

all: $(TARGET)

$(TARGET): $(addprefix $(BUILD_DIR)/, $(OBJECTS))
	$(CC) $^ -o $(TARGET) $(LFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -rf $(BUILD_DIR)
