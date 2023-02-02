#include <blackhv/atapi.h>
#include <blackhv/memory.h>
#include <blackhv/serial.h>
#include <blackhv/vm.h>
#include <elf.h>
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

#include "multiboot.h"

#define MULTIBOOT_GUEST 0xc10000
#define CMDLINE "/bin/hunter"
#define CMDLINE_GUEST 0xc20000
#define MEMORY_GUEST 0xc30000

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

static void setup_e820(vm_t *vm, multiboot_info_t *info)
{
    struct e820_table *table = e820_table_get(vm);

    info->mmap_addr = MEMORY_GUEST;
    multiboot_memory_map_t *guest_memory = memory_get_ptr(vm, MEMORY_GUEST);
    if (guest_memory == NULL)
    {
        errx(1, "Could not get MEMORY_GUEST pointer");
    }

    if (table == NULL)
    {
        errx(1, "failed to get e820 table");
    }

    for (size_t i = 0; i < table->length; ++i)
    {
        guest_memory[i].size = sizeof(multiboot_memory_map_t);
        guest_memory[i].addr = table->entries[i].base_address;
        guest_memory[i].len = table->entries[i].size;
        guest_memory[i].type = table->entries[i].type;
    }

    info->mmap_length = table->length * sizeof(multiboot_memory_map_t);

    info->mem_lower = 0x1000;

    e820_table_free(table);
}

static void load_k(vm_t *vm, char *image, size_t image_size)
{
    multiboot_info_t *multiboot =
        (multiboot_info_t *)memory_get_ptr(vm, MULTIBOOT_GUEST);
    if (multiboot == NULL)
    {
        errx(1, "Could not get multiboot guest pointer");
    }

    memset(multiboot, 0, sizeof(multiboot_info_t));
    setup_e820(vm, multiboot);

    char *cmdline = (char *)memory_get_ptr(vm, CMDLINE_GUEST);
    if (cmdline == NULL)
    {
        errx(1, "Could not get cmdline guest pointer");
    }

    strcpy(cmdline, CMDLINE);
    multiboot->cmdline = (multiboot_uint32_t)CMDLINE_GUEST;

    Elf32_Ehdr *header = (Elf32_Ehdr *)image;
    Elf32_Phdr *program_headers = (Elf32_Phdr *)(image + header->e_phoff);
    for (size_t i = 0; i < header->e_phnum; i++)
    {
        if ((program_headers[i].p_type & PT_LOAD) != 0)
        {
            char *mem = memory_get_ptr(vm, program_headers[i].p_paddr);
            if (mem == NULL)
            {
                errx(1, "Could not get program header guest pointer");
            }
            memset(mem, 0, program_headers[i].p_memsz);
            memcpy(mem,
                   image + program_headers[i].p_offset,
                   program_headers[i].p_filesz);
        }
    }

    struct kvm_regs regs;

    if (vm_get_regs(vm, &regs) != 1)
    {
        errx(1, "Failed to get regs");
    }

    regs.rbx = MULTIBOOT_GUEST;
    regs.rax = MULTIBOOT_HEADER_MAGIC;
    regs.rip = header->e_entry;

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
    printf("Serial thread started\n");
    serial_t *serial = (serial_t *)param;
    unsigned char buffer[1024];

    for (;;)
    {
        int r = (int)serial_read(serial, buffer, 1024);

        printf("%.*s", r, buffer);
    }

    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        errx(1, "Usage: ./k_example /path/to/k.elf disk.iso");
    }

    vm_t *vm = init_vm();

    if (memory_alloc(vm, 0x0, GB_1, MEMORY_USABLE) != 1)
    {
        errx(1, "Failed to allocate memory");
    }

    size_t image_size = 0;
    char *image = load_file(argv[1], &image_size);

    load_k(vm, image, image_size);

    serial_t *serial = serial_new(COM1, 1024);

    if (serial == NULL)
    {
        errx(1, "Failed to create serial");
    }

    pthread_t pthread;

    pthread_create(&pthread, NULL, serial_thread, serial);

    int disk_fd = open(argv[2], O_RDONLY);
    atapi_init(vm, disk_fd);

    sleep(1);

    printf("Running the VM\n");

    if (vm_run(vm) == 0)
    {
        errx(1, "Failed to run VM");
    }

    close(disk_fd);
    vm_destroy(vm);

    return 0;
}