#include <blackhv/memory.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

static struct memory_entry *find_entry(vm_t *vm, u64 addr)
{
    struct linked_list_elt *current = vm->mem->memory_entries->head;

    while (current != NULL)
    {
        struct memory_entry *entry = (struct memory_entry *)current->value;

        if (entry->guest_phys <= addr
            && (entry->guest_phys + entry->size) > addr)
        {
            return entry;
        }

        current = current->next;
    }

    return NULL;
}

static struct memory_entry *allocate_usable(vm_t *vm, u64 phys_addr, u64 size)
{
    struct memory_entry *entry = malloc(sizeof(struct memory_entry));

    if (entry == NULL)
    {
        return NULL;
    }

    // Allocate the memory
    void *mem_ptr = mmap(NULL,
                         size,
                         PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
                         -1,
                         0);

    if (mem_ptr == NULL)
    {
        free(entry);
        return NULL;
    }

    // Add the memory to vm
    struct kvm_userspace_memory_region region = { .slot = vm->mem->next_slot,
                                                  .flags = 0,
                                                  .guest_phys_addr = phys_addr,
                                                  .memory_size = size,
                                                  .userspace_addr =
                                                      (u64)mem_ptr };

    if (ioctl(vm->vm_fd, KVM_SET_USER_MEMORY_REGION, &region) == -1)
    {
        munmap(mem_ptr, size);
        free(entry);
        return NULL;
    }

    entry->guest_phys = phys_addr;
    entry->memory_ptr = mem_ptr;
    entry->size = size;
    entry->slot = vm->mem->next_slot;
    entry->type = MEMORY_USABLE;

    vm->mem->next_slot += 1;

    return entry;
}

static struct memory_entry *allocate_mmio(u64 phys, u64 size)
{
    struct memory_entry *entry = malloc(sizeof(struct memory_entry));

    if (entry == NULL)
    {
        return NULL;
    }

    entry->guest_phys = phys;
    entry->memory_ptr = NULL;
    entry->size = size;
    entry->slot = 0;
    entry->type = MEMORY_MMIO;

    return entry;
}

static s32 check_addr_available(vm_t *vm, u64 phys_addr, u64 size)
{
    struct linked_list_elt *current = vm->mem->memory_entries->head;

    while (current != NULL)
    {
        struct memory_entry *entry = (struct memory_entry *)current->value;

        if ((entry->guest_phys <= phys_addr
             && (entry->guest_phys + entry->size) > phys_addr)
            || ((phys_addr + size) >= entry->guest_phys
                && phys_addr < entry->guest_phys))
        {
            return 0;
        }

        current = current->next;
    }

    return 1;
}

s32 memory_alloc(vm_t *vm, u64 phys_addr, u64 size, u32 type)
{
    if (vm == NULL || check_addr_available(vm, phys_addr, size) == 0)
    {
        return 0;
    }

    struct memory_entry *entry = NULL;
    switch (type)
    {
    case MEMORY_USABLE:
        entry = allocate_usable(vm, phys_addr, size);
        break;
    case MEMORY_MMIO:
        entry = allocate_mmio(phys_addr, size);
        break;
    }

    if (entry == NULL)
    {
        return 0;
    }

    linked_list_add(vm->mem->memory_entries, entry);

    return 1;
}

s64 memory_write(vm_t *vm, u64 dest, u8 *buffer, u64 size)
{
    struct memory_entry *entry = find_entry(vm, dest);

    if (entry == NULL || entry->type != MEMORY_USABLE)
    {
        return -1;
    }

    u64 base_address = dest - entry->guest_phys;
    u64 mem_limit = entry->size - base_address;
    u8 *ptr = (u8 *)entry->memory_ptr;
    u64 written = 0;

    for (; written < mem_limit && written < size; ++written)
    {
        ptr[base_address + written] = buffer[written];
    }

    return (s64)written;
}

void *memory_get_ptr(vm_t *vm, u64 addr)
{
    struct memory_entry *entry = find_entry(vm, addr);

    if (entry == NULL)
    {
        return 0x0;
    }

    return ((u8 *)entry->memory_ptr) + addr;
}

struct e820_table *e820_table_get(vm_t *vm)
{
    struct e820_table *table = malloc(sizeof(struct e820_table));

    if (table == NULL)
    {
        return NULL;
    }

    table->length = linked_list_size(vm->mem->memory_entries);
    table->entries = malloc(sizeof(struct e820_entry) * table->length);

    if (table->entries == NULL)
    {
        free(table);
        return NULL;
    }

    struct linked_list_elt *current = vm->mem->memory_entries->head;

    for (size_t i = 0; current != NULL && i < table->length; ++i)
    {
        struct memory_entry *map = (struct memory_entry *)current->value;

        table->entries[i].base_address = map->guest_phys;
        table->entries[i].size = map->size;
        table->entries[i].type =
            map->type == MEMORY_USABLE ? E820_USABLE : E820_RESERVED;

        current = current->next;
    }

    return table;
}

void e820_table_free(struct e820_table *table)
{
    if (table == NULL)
    {
        return;
    }

    free(table->entries);
    free(table);
}

static void free_memory_entry(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }

    struct memory_entry *entry = (struct memory_entry *)ptr;

    if (entry->type == MEMORY_USABLE)
    {
        munmap(entry->memory_ptr, entry->size);
    }

    free(entry);
}

memory_t *memory_new()
{
    memory_t *mem = malloc(sizeof(memory_t));

    if (mem == NULL)
    {
        return NULL;
    }

    mem->next_slot = 0;
    mem->memory_entries = linked_list_new(free_memory_entry);

    if (mem->memory_entries == NULL)
    {
        free(mem);
        return NULL;
    }

    return mem;
}

void memory_destroy(memory_t *mem)
{
    if (mem == NULL)
    {
        return;
    }

    linked_list_free(mem->memory_entries);
    free(mem);
}