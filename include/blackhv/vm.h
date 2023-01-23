#ifndef VM_HEADER
#define VM_HEADER

#include <blackhv/linked_list.h>
#include <blackhv/memory.h>
#include <blackhv/types.h>
#include <linux/kvm.h>

#define MB_1 1048576

typedef struct memory memory_t;

typedef struct vm
{
    s32 kvm_fd;
    s32 vm_fd;
    s32 vcpu_fd;
    struct kvm_run *kvm_run;
    memory_t *mem;

} vm_t;

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
 * Run the virtual memory
 *
 * @param vm the vm to run
 * @return 1 on success, 0 otherwise
 */
s32 vm_run(vm_t *vm);

s32 vm_dump_regs(vm_t *vm);

#endif
