#define _GNU_SOURCE

#include <blackhv/memory.h>
#include <blackhv/mmio.h>
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

void mmio_write_handler(struct mmio_region *region,
                        u64 address,
                        u8 data[8],
                        u32 len,
                        void *arg)
{
    (void)arg;
    printf(
        "mmio_id: %d address: %llx len: %u data: ", region->id, address, len);

    for (u32 i = 0; i < len; ++i)
    {
        printf("%x ", data[i]);
    }

    printf("\n");
}

void *worker0(void *params)
{
    (void)params;

    serial_t *serial = (serial_t *)params;

    char *line = NULL;
    size_t buffer_len = 0;

    for (;;)
    {
        ssize_t read_len = getline(&line, &buffer_len, stdin);
        if (read_len == -1)
        {
            errx(1, "getline failed");
        }

        serial_write(serial, (u8 *)line, read_len);
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

void dump_e820_table(vm_t *vm)
{
    struct e820_table *e820_table = e820_table_get(vm);

    if (e820_table == NULL)
    {
        return;
    }

    printf("e820 table:\n");
    for (size_t i = 0; i < e820_table->length; ++i)
    {
        printf("base address: %llx size: %llx type: %u\n",
               e820_table->entries[i].base_address,
               e820_table->entries[i].size,
               e820_table->entries[i].type);
    }

    printf("\n");

    e820_table_free(e820_table);
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

    if (memory_alloc(vm, 0x0, MB_1, MEMORY_USABLE) == 0)
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
        if (memory_write(vm, START_ADDRESS + readed, buffer, r) < 0)
        {
            errx(1,
                 "Failed to load the binary into memory %ld",
                 START_ADDRESS + readed);
        }

        readed += r;
    }

    close(fd);

    mmio_init();

    struct mmio_region region = { .base_address = 0xC0000000,
                                  .high_address = 0xC1000000,
                                  .write_handler = mmio_write_handler,
                                  .read_handler = NULL,
                                  .data = NULL };

    s32 mmio_region_id = mmio_register(vm, &region);

    if (mmio_region_id < 0)
    {
        errx(1, "Failed to register a mmio region");
    }

    printf("Launching the VM\n");

    serial_t *serial = serial_new(COM1, 1024);

    pthread_t th0;
    pthread_create(&th0, NULL, worker0, serial);

    pthread_t th1;
    pthread_create(&th1, NULL, worker1, serial);

    dump_e820_table(vm);

    screen_init(vm, 0xC2000000);
    pthread_t th2;
    pthread_create(&th2, NULL, screen_run, (void *)vm);

    printf("VESA initialized\n");

    sleep(1);

    if (vm_run(vm) != 1)
    {
        errx(1, "Failed to run VM\n");
    }

    pthread_join(th0, NULL);
    pthread_join(th1, NULL);
    pthread_join(th2, NULL);

    screen_uninit(vm);
    vm_destroy(vm);

    return 0;
}
