#include <asm/bootparam.h>
#include <blackhv/memory.h>
#include <blackhv/serial.h>
#include <blackhv/vm.h>
#include <err.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BOOT_PARAMS_ADDR 0x10000
#define REAL_MODE_ADDR 0x90000
#define KERNEL_ADDR 0x100000
#define CMD_LINE "console=ttyS0"

static vm_t *init_vm()
{
    vm_t *vm = vm_new();

    if (vm == NULL)
    {
        errx(1, "Failed to init VM");
    }

    // start address is updated later
    if (vm_vcpu_init_state(vm,
                           0xffffd000,
                           0xffffc000,
                           PROTECTED_MODE | CREATE_IRQCHIP | CREATE_PIT)
        == 0)
    {
        errx(1, "Failed to initialize vcpu state");
    }

    return vm;
}

static void setup_e820(vm_t *vm, struct boot_params *params)
{
    struct e820_table *table = e820_table_get(vm);

    if (table == NULL)
    {
        errx(1, "failed to get e820 table");
    }

    for (size_t i = 0; i < table->length; ++i)
    {
        params->e820_table[i].addr = table->entries[i].base_address;
        params->e820_table[i].size = table->entries[i].size;
        params->e820_table[i].type = table->entries[i].type;
    }

    params->e820_entries = table->length;

    e820_table_free(table);
}

static void set_setup_header(struct boot_params *boot_params)
{
    boot_params->hdr.vid_mode = 0xFFFF; // VGA
    boot_params->hdr.type_of_loader = 0xFF;
    boot_params->hdr.ramdisk_image = 0x0;
    boot_params->hdr.ramdisk_size = 0x0;
    boot_params->hdr.ext_loader_ver = 0x0;
    boot_params->hdr.ext_loader_type = 0x0;

    boot_params->hdr.loadflags |= CAN_USE_HEAP | LOADED_HIGH | KEEP_SEGMENTS;
    boot_params->hdr.heap_end_ptr =
        0xe000 - 0x200; // We assume protocol version >= 0x202
    boot_params->hdr.cmd_line_ptr =
        REAL_MODE_ADDR + boot_params->hdr.heap_end_ptr;
}

static void load_image_into_memory(vm_t *vm,
                                   struct boot_params *boot_params,
                                   char *image,
                                   size_t image_size)
{
    char *kernel = memory_get_ptr(vm, KERNEL_ADDR);

    if (kernel == NULL)
    {
        errx(1, "Failed to get kernel ptr from VM");
    }

    size_t setup_sectors = boot_params->hdr.setup_sects;

    if (setup_sectors < 4)
    {
        setup_sectors = 4;
    }

    size_t setup_size = (setup_sectors + 1) * 512;
    size_t kernel_size = image_size - setup_size;

    // Copy the kernel
    memcpy(kernel, image + setup_size, kernel_size);
}

static void load_linux(vm_t *vm, char *image, size_t image_size)
{
    struct boot_params *boot_params = memory_get_ptr(vm, BOOT_PARAMS_ADDR);

    if (boot_params == NULL)
    {
        errx(1, "Failed to get boot_params ptr from VM");
    }

    memset(boot_params, 0, sizeof(struct boot_params));
    memcpy(boot_params, image, sizeof(struct boot_params));

    set_setup_header(boot_params);
    setup_e820(vm, boot_params);

    // Now let's copy the cmd line into the vm memory
    char *cmdline = memory_get_ptr(vm, boot_params->hdr.cmd_line_ptr);

    if (cmdline == NULL)
    {
        errx(1, "Failed to get cmdline ptr from VM");
    }

    memset(cmdline, 0, boot_params->hdr.cmdline_size);
    memcpy(cmdline, CMD_LINE, strlen(CMD_LINE));

    load_image_into_memory(vm, boot_params, image, image_size);

    struct kvm_regs regs;

    if (vm_get_regs(vm, &regs) != 1)
    {
        errx(1, "Failed to get regs");
    }

    regs.rsi = BOOT_PARAMS_ADDR;
    regs.rip = boot_params->hdr.code32_start;

    if (vm_set_regs(vm, &regs) != 1)
    {
        errx(1, "Failed to set regs");
    }
}

static void *load_file(const char *path, size_t *size)
{
    int fd = open(path, O_RDONLY);

    if (fd < 0)
    {
        return NULL;
    }

    struct stat stat;

    if (fstat(fd, &stat) < 0)
    {
        close(fd);
        return NULL;
    }

    char *map =
        mmap(0x0, stat.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);

    close(fd);

    if (map == MAP_FAILED)
    {
        close(fd);
        return NULL;
    }

    *size = stat.st_size;
    return map;
}

static void *serial_thread(void *param)
{
    printf("Thread started\n");
    serial_t *serial = (serial_t *)param;
    unsigned char buffer[1024];

    for (;;)
    {
        int r = (int)serial_read(serial, buffer, 1024);

        printf("%.*s", r, buffer);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        errx(1, "invalid args: ./linux_example <bzimage>");
    }

    vm_t *vm = init_vm();

    if (memory_alloc(vm, 0x0, GB_1, MEMORY_USABLE) != 1)
    {
        errx(1, "Failed to allocate memory");
    }

    size_t image_size = 0;
    char *image = load_file(argv[1], &image_size);

    load_linux(vm, image, image_size);

    serial_t *serial = serial_new(COM1, 1024);

    if (serial == NULL)
    {
        errx(1, "Failed to create serial");
    }

    pthread_t pthread;

    pthread_create(&pthread, NULL, serial_thread, serial);

    sleep(1);

    printf("Running the VM\n");

    if (vm_run(vm) == 0)
    {
        errx(1, "Failed to run VM");
    }

    vm_destroy(vm);

    return 0;
}
