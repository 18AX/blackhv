CC?=gcc
CFLAGS+=-Wall -Wextra -pedantic -I../../include/

SDL_LDFLAGS=`sdl2-config --libs`

LFLAGS=-lasan -lpthread -L../../build/ -lblackhv $(SDL_LDFLAGS)

BUILD_DIR=build

TARGET=$(BUILD_DIR)/k_example

OBJECTS=main.o

all: $(TARGET)

$(TARGET): $(addprefix $(BUILD_DIR)/, $(OBJECTS))
	$(CC) $^ -o $(TARGET) $(LFLAGS)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -r $(BUILD_DIR)