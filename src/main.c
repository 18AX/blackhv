#define _GNU_SOURCE

#include <blackhv/serial.h>
#include <blackhv/vm.h>
#include <err.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define START_ADDRESS 0x7c00

void *worker0(void *params)
{
    (void)params;

    serial_t *serial = (serial_t *)params;

    char *line = NULL;
    size_t len = 0;

    for (;;)
    {
        if (getline(&line, &len, stdin) == -1)
        {
            errx(1, "getline failed");
        }

        serial_write(serial, (u8 *)line, len);
    }
}

void *worker1(void *params)
{
    serial_t *serial = (serial_t *)params;

    for (;;)
    {
        char c;

        if (serial_read(serial, (u8 *)&c, 1) == 1)
        {
            putchar(c);
        }
    }
}

int main(int argc, const char *argv[])
{
    if (argc != 2)
    {
        errx(1, "usage: blackhv <binary>");
    }

    int fd = open(argv[1], O_RDONLY);

    if (fd < 0)
    {
        errx(1, "Failed to open %s", argv[1]);
    }

    vm_t *vm = vm_new();

    // Vm creation failed
    if (vm == NULL)
    {
        errx(1, "Failed to create a vm object");
    }

    s32 init = vm_vcpu_init_state(vm,
                                  START_ADDRESS,
                                  0xffffd000,
                                  0xffffc000,
                                  PROTECTED_MODE | CREATE_IRQCHIP | CREATE_PIT);

    if (init == 0)
    {
        errx(1, "Failed to initialize virtual cpu");
    }

    if (vm_alloc_memory(vm, 0x0, MB_1) == 0)
    {
        errx(1, "Failed to allocate 1 Mb of memory");
    }

    printf("Vm created\n");
    printf("Vcpu initialized\n");

    ssize_t readed = 0;
    ssize_t r;

    u8 buffer[BUFFER_SIZE];

    while ((r = read(fd, buffer, BUFFER_SIZE)) > 0)
    {
        if (vm_memory_write(vm, START_ADDRESS + readed, buffer, r) < 0)
        {
            errx(1, "Failed to load the binary into memory");
        }

        readed += r;
    }

    close(fd);

    printf("Launching the VM\n");

    serial_t *serial = serial_new(COM1, 1024);

    pthread_t th0;
    pthread_create(&th0, NULL, worker0, serial);

    pthread_t th1;
    pthread_create(&th1, NULL, worker1, serial);

    sleep(1);

    if (vm_run(vm) != 1)
    {
        errx(1, "Failed to run VM\n");
    }

    pthread_join(th0, NULL);
    pthread_join(th1, NULL);

    vm_destroy(vm);

    return 0;
}
