CC=gcc
CFLAGS=-m32 -fno-pie -nostdinc -nostdlib -ffreestanding -mno-red-zone -std=gnu99 -Wall -pedantic -Werror -Wextra
LFLAGS=-m elf_i386 -nostdlib -T guest.ld

OBJECT16=guest16.S.o
OBJECT32=guest32.c.o

.PHONY: all clean

all: guest16.bin guest32.bin example

guest16.bin: $(OBJECT16)
	ld $(LFLAGS) $(OBJECT16) -o guest16.bin

guest32.bin: $(OBJECT32)
	ld  $(LFLAGS) $(OBJECT32) -o guest32.bin

example: main.c
	$(CC) main.c -I../../include/ -o example -lpthread -L../../build/ -lblackhv -lasan

%.c.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.S.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	$(RM) $(OBJECT16) $(OBJECT32) guest16.bin guest32.bin *.o example
