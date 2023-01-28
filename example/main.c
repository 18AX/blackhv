#include <asm/bootparam.h>
#include <blackhv/memory.h>
#include <blackhv/serial.h>
#include <blackhv/vm.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BOOT_PARAMS_ADDR 0x10000
#define REAL_MODE_LOAD_ADDR 0x90000
#define KERNEL_ADDR 0x100000

static vm_t *init_vm()
{
    vm_t *vm = vm_new();

    if (vm == NULL)
    {
        errx(1, "Failed to init VM");
    }

    // start address is updated later
    if (vm_vcpu_init_state(vm,
                           0x0,
                           0xffffd000,
                           0xffffc000,
                           PROTECTED_MODE | CREATE_IRQCHIP | CREATE_PIT)
        == 0)
    {
        errx(1, "Failed to initialize vcpu state");
    }

    return vm;
}

static void setup_header(struct setup_header *hdr)
{
    hdr->boot_flag |= CAN_USE_HEAP | LOADED_HIGH;

    // For now we dont load a ramdisk
    hdr->ramdisk_image = 0;
    hdr->ram_size = 0;

    hdr->heap_end_ptr = 0xe000 - 0x200; // We assume that protocol >= 0x202
    hdr->cmd_line_ptr = REAL_MODE_LOAD_ADDR + hdr->heap_end_ptr;

    hdr->type_of_loader = 0xFF;
    hdr->ext_loader_type = 0;
    hdr->ext_loader_ver = 0;
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

static void copy_image_into_memory(vm_t *vm,
                                   struct setup_header *hdr,
                                   char *image,
                                   size_t image_size)
{
    size_t sectors = hdr->setup_sects;

    if (sectors == 0)
    {
        sectors = 4;
    }

    size_t setup_size = (sectors + 1) * 512;

    size_t kernel_size = image_size - setup_size;

    memory_write(vm, REAL_MODE_LOAD_ADDR, (unsigned char *)image, setup_size);
    memory_write(
        vm, KERNEL_ADDR, (unsigned char *)image + setup_size, kernel_size);
}

static void load_linux(vm_t *vm, char *image, size_t image_size)
{
    (void)vm;
    struct boot_params *boot_params = malloc(sizeof(struct boot_params));

    memset(boot_params, 0x0, sizeof(struct boot_params));

    // Copy the setup header
    memcpy(&boot_params->hdr, image + 0x1F1, sizeof(struct setup_header));

    setup_header(&boot_params->hdr);
    setup_e820(vm, boot_params);

    copy_image_into_memory(vm, &boot_params->hdr, image, image_size);

    printf("sentinel %x\n", boot_params->sentinel);
    printf("code32_start 0x%x", boot_params->hdr.code32_start);

    free(boot_params);
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

    int fd = open(argv[1], O_RDONLY);

    if (fd < 0)
    {
        errx(1, "Failed to open %s", argv[1]);
    }

    struct stat stat;

    if (fstat(fd, &stat) < 0)
    {
        errx(1, "Failed to fstat");
    }

    char *map = mmap(0x0, stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    if (map == MAP_FAILED)
    {
        errx(1, "mmap failed");
    }

    load_linux(vm, map, stat.st_size);

    vm_destroy(vm);

    return 0;
}
