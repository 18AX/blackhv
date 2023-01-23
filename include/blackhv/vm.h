#ifndef VM_HEADER
#define VM_HEADER

#include <blackhv/linked_list.h>
#include <blackhv/types.h>
#include <linux/kvm.h>

#define MB_1 1048576

typedef struct
{
    s32 kvm_fd;
    s32 vm_fd;
    s32 vcpu_fd;
    struct kvm_run *kvm_run;
    linked_list_t *memory_list;

} vm_t;

struct memory_mapped
{
    vm_t *vm;
    u32 slot;
    void *ptr;
    u64 guest_phys;
    u64 size;
};

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

/**
 * Allocate an new vm_t object. vm_t object can be freed using vm_destroy
 *
 * @return a new vm_t object
 */
vm_t *vm_new(void);

/**
 * Destroy a vm_t object.
 *
 * @param vm the vm to free, can be NULL
 */
void vm_destroy(vm_t *vm);

/** Flags for vm_vcpu_init_state **/
#define REAL_MODE 0x1
#define PROTECTED_MODE (0x1 << 1)
#define CREATE_IRQCHIP (0x1 << 2)
#define CREATE_PIT (0x1 << 3)

/**
 * Initiate virtual cpu
 *
 * @param vm the virtual machine structure
 * @param code_addr address of code executed by vm
 * @param tss_address physical address of a 3 pages region in the guest. The
 * address must not conflict with any mmio or memory slot
 * @param flags intialization flags
 */
s32 vm_vcpu_init_state(vm_t *vm,
                       u64 code_addr,
                       u64 tss_address,
                       u64 identity_map_address,
                       u32 flags);

/**
 * Allocate the memory that will be usable by the guest
 *
 * @param size the size of the memory area (should be aligned on 4kb)
 * @return 0 on error, 1 otherwise
 */
s32 vm_alloc_memory(vm_t *vm, u64 phys_addr, u64 size);

/**
 * Write into guest memory area
 *
 * @param vm vm to write to
 * @param buffer input data
 * @param size size in bytes
 */
s64 vm_memory_write(vm_t *vm, u64 dest, u8 *buffer, u64 size);

/**
 * Get e820 table of a vm.
 *
 * @param vm
 * @return an allocated struct e820_table on success, NULL otherwise.
 */
struct e820_table *vm_e820_table_get(vm_t *vm);

void vm_e820_table_free(struct e820_table *table);

/**
 * Run the virtual memory
 *
 * @param vm the vm to run
 * @return 1 on success, 0 otherwise
 */
s32 vm_run(vm_t *vm);

s32 vm_dump_regs(vm_t *vm);

#endif
