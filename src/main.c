#include <blackhv/vm.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define START_ADDRESS 0x7c00

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

    if (vm_alloc_memory(vm, 0x0, MB_1) == 0)
    {
        errx(1, "Failed to allocate 1 Mb of memory");
    }

    printf("Vm created\n");

    s32 init = vm_vcpu_set_state(vm, START_ADDRESS, PROTECTED_MODE);
    if (init == -1)
    {
        errx(1, "Failed to initialize virtual cpu");
    }

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

    if (vm_run(vm) != 1)
    {
        errx(1, "Failed to run VM\n");
    }

    vm_destroy(vm);

    return 0;
}
