#ifndef MEMORY_HEADER
#define MEMORY_HEADER

#include <blackhv/linked_list.h>
#include <blackhv/types.h>
#include <blackhv/vm.h>

#define KB_1 (1 << 10)
#define MB_1 (1 << 20)
#define GB_1 (1 << 30)

#define MEMORY_USABLE 0x1
#define MEMORY_MMIO 0x2

typedef struct vm vm_t;

struct memory_entry
{
    void *memory_ptr;
    u64 guest_phys;
    u64 size;
    u32 slot;
    u32 type;
};

struct memory
{
    linked_list_t *memory_entries;
    u32 next_slot;
};

typedef struct memory memory_t;

#define E820_USABLE 0x1
#define E820_RESERVED 0x2
#define E820_ACPI_RECLAIMED 0x3
#define E820_ACPI_NVS 0x4
#define E820_BAD_MEMORY 0x5

struct e820_entry
{
    u64 base_address;
    u64 size;
    u32 type;
};

struct e820_table
{
    struct e820_entry *entries;
    u64 length;
};

memory_t *memory_new(void);

void memory_destroy(memory_t *mem);

s32 memory_alloc(vm_t *vm, u64 phys_addr, u64 size, u32 type);

/**
 * Write into guest memory area
 *
 * @param vm vm to write to
 * @param buffer input data
 * @param size size in bytes
 */
s64 memory_write(vm_t *vm, u64 dest, u8 *buffer, u64 size);

/**
 * Get e820 table of a vm.
 *
 * @param vm
 * @return an allocated struct e820_table on success, NULL otherwise.
 */
struct e820_table *e820_table_get(vm_t *vm);

void e820_table_free(struct e820_table *table);

#endif