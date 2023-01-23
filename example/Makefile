CC?=gcc
CFLAGS+=-Wall -Werror -Wextra -pedantic -I../include/
LFLAGS=-lasan -L../build/ -lblackhv

BUILD_DIR=build

TARGET=$(BUILD_DIR)/linux_example

OBJECTS=main.o

all: $(TARGET)

$(TARGET): $(addprefix $(BUILD_DIR)/, $(OBJECTS))
	$(CC) $^ -o $(TARGET) $(LFLAGS)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -r $(BUILD_DIR)