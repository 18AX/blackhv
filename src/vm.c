#include <blackhv/cpu.h>
#include <blackhv/io.h>
#include <blackhv/mmio.h>
#include <blackhv/vm.h>
#include <err.h>
#include <fcntl.h>
#include <linux/kvm.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void free_memory_mapped(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }
    struct memory_mapped *m = (struct memory_mapped *)ptr;

    // TODO: Not working for an unknown reason
#if 0
    struct kvm_userspace_memory_region d = {
        .slot = m->slot,
        .flags = 0,
        .guest_phys_addr = m->guest_phys,
        .memory_size = 0,
        .userspace_addr = (u64) m->ptr
    };

    if (ioctl(m->vm->vm_fd, KVM_SET_USER_MEMORY_REGION, d) == -1)
    {
        errx(1, "Failed to clear memory region fd=%d %u slot", m->vm->vm_fd, m->slot);
    }
#endif
    munmap(m->ptr, m->size);
    free(ptr);
}

vm_t *vm_new()
{
    // We open kvm char device
    s32 fd = open("/dev/kvm", O_RDWR);

    if (fd < 0)
    {
        return NULL;
    }

    // Create a vm fd
    s32 vm_fd = ioctl(fd, KVM_CREATE_VM, 0);

    if (vm_fd < 0)
    {
        return NULL;
    }

    vm_t *vm = malloc(sizeof(vm_t));
    memset(vm, 0, sizeof(vm_t));

    if (vm == NULL)
    {
        close(fd);
        return NULL;
    }

    vm->kvm_fd = fd;
    vm->vm_fd = vm_fd;
    vm->memory_list = linked_list_new(free_memory_mapped);

    if (vm->memory_list == NULL)
    {
        close(fd);
        free(vm);
        return NULL;
    }

    return vm;
}

void vm_destroy(vm_t *vm)
{
    if (vm == NULL)
    {
        return;
    }

    linked_list_free(vm->memory_list);
    close(vm->kvm_fd);
    close(vm->vm_fd);
    free(vm);
}

static s32 set_real_mode(vm_t *vm, u64 code_addr)
{
    struct kvm_sregs sregs;
    if (ioctl(vm->vcpu_fd, KVM_GET_SREGS, &sregs) < 0)
    {
        return -1;
    }

    sregs.cs.selector = 0;
    sregs.cs.base = 0;

    if (ioctl(vm->vcpu_fd, KVM_SET_SREGS, &sregs) < 0)
    {
        return -1;
    }

    struct kvm_regs regs;

    memset(&regs, 0, sizeof(regs));
    regs.rflags = 2;
    regs.rip = code_addr;

    if (ioctl(vm->vcpu_fd, KVM_SET_REGS, &regs) < 0)
    {
        return -1;
    }

    return 0;
}

static s32 set_protected_mode(vm_t *vm, u64 code_addr)
{
    (void)vm;
    (void)code_addr;
    // Not implemented yet

    struct kvm_sregs sregs;

    if (ioctl(vm->vcpu_fd, KVM_GET_SREGS, &sregs) < 0)
    {
        return 0;
    }

    struct kvm_segment code = {
        .base = 0x0,
        .limit = 0xFFFFFFFF,
        .selector = SEGMENT_SELECTOR(1, 0, 0),
        .type = GDT_TYPE_RD | GDT_TYPE_EXEC,
        .s = 1, // Code or data
        .dpl = 0,
        .present = 1, // Present
        .avl = 0,
        .l = 0,
        .db = 1, // 32 bit
        .g = 1, // Granularity 4kb
    };

    struct kvm_segment data = {
        .base = 0x0,
        .limit = 0xFFFFFFFF,
        .selector = SEGMENT_SELECTOR(2, 0, 0),
        .type = GDT_TYPE_WR,
        .s = 1, // Code or data
        .dpl = 0,
        .present = 1, // Present
        .avl = 0,
        .l = 0,
        .db = 1, // 32 bit
        .g = 1, // Granularity 4kb
    };

    sregs.cr0 |= CR0_PE;
    sregs.cs = code;
    sregs.ss = data;
    sregs.ds = data;
    sregs.es = data;
    sregs.fs = data;
    sregs.gs = data;

    if (ioctl(vm->vcpu_fd, KVM_SET_SREGS, &sregs) < 0)
    {
        return 0;
    }

    struct kvm_regs regs;

    memset(&regs, 0, sizeof(regs));
    regs.rflags = 2;
    regs.rip = code_addr;

    if (ioctl(vm->vcpu_fd, KVM_SET_REGS, &regs) < 0)
    {
        return 0;
    }

    return 1;
}

#include <stdio.h>
s32 vm_vcpu_init_state(vm_t *vm,
                       u64 code_addr,
                       u64 tss_address,
                       u64 identity_map_addr,
                       u32 mode)
{
    if (ioctl(vm->vm_fd, KVM_SET_TSS_ADDR, tss_address, 0) < 0
        || ioctl(vm->vm_fd, KVM_SET_IDENTITY_MAP_ADDR, &identity_map_addr, 0)
            < 0)
    {
        printf("JERE\n");
        return 0;
    }

    if ((mode & CREATE_IRQCHIP) != 0
        && ioctl(vm->vm_fd, KVM_CREATE_IRQCHIP, 0) < 0)
    {
        return 0;
    }

    if ((mode & CREATE_PIT) != 0)
    {
        struct kvm_pit_config pit_config = { 0 };

        if (ioctl(vm->vm_fd, KVM_CREATE_PIT2, &pit_config, 0) < 0)
        {
            return 0;
        }
    }

    vm->vcpu_fd = ioctl(vm->vm_fd, KVM_CREATE_VCPU, 0);
    if (vm->vcpu_fd < 0)
    {
        return 0;
    }

    s32 vcpu_size = ioctl(vm->kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
    if (vcpu_size <= 0)
    {
        return 0;
    }

    vm->kvm_run = mmap(
        NULL, vcpu_size, PROT_READ | PROT_WRITE, MAP_SHARED, vm->vcpu_fd, 0);
    if (vm->kvm_run == MAP_FAILED)
    {
        return 0;
    }

    if ((mode & REAL_MODE) != 0)
    {
        return set_real_mode(vm, code_addr);
    }
    else if ((mode & PROTECTED_MODE) != 0)
    {
        return set_protected_mode(vm, code_addr);
    }

    return 1;
}

static u32 next_slot = 0;

s32 vm_alloc_memory(vm_t *vm, u64 phys_addr, u64 size)
{
    // Allocate the memory
    void *mem_ptr = mmap(NULL,
                         size,
                         PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
                         -1,
                         0);

    if (mem_ptr == NULL)
    {
        return 0;
    }

    // Add the memory to vm
    struct kvm_userspace_memory_region region = { .slot = next_slot,
                                                  .flags = 0,
                                                  .guest_phys_addr = phys_addr,
                                                  .memory_size = size,
                                                  .userspace_addr =
                                                      (u64)mem_ptr };

    if (ioctl(vm->vm_fd, KVM_SET_USER_MEMORY_REGION, &region) == -1)
    {
        munmap(mem_ptr, size);
        return 0;
    }

    next_slot = next_slot + 1;

    struct memory_mapped *m = malloc(sizeof(struct memory_mapped));

    if (m == NULL)
    {
        munmap(mem_ptr, size);
        return 0;
    }

    m->slot = region.slot;
    m->vm = vm;
    m->ptr = mem_ptr;
    m->guest_phys = phys_addr;
    m->size = size;

    if (linked_list_add(vm->memory_list, m) == 0)
    {
        munmap(mem_ptr, size);
        free(m);
        return 0;
    }

    return 1;
}

s64 vm_memory_write(vm_t *vm, u64 dest_addr, u8 *buffer, u64 size)
{
    if (vm == NULL)
    {
        return -1;
    }

    struct linked_list_elt *curr = vm->memory_list->head;

    // try to find the destination memory
    while (curr != NULL)
    {
        struct memory_mapped *m = (struct memory_mapped *)curr->value;

        if (dest_addr >= m->guest_phys && dest_addr < m->guest_phys + m->size)
        {
            // Copy buffer to destination
            s64 written = 0;
            s64 mem_limit = (s64)(m->size - dest_addr);

            u64 base_addr = dest_addr - m->guest_phys;

            for (; written < mem_limit && written < (s64)size; ++written)
            {
                ((u8 *)m->ptr)[base_addr + written] = buffer[written];
            }

            return written;
        }

        curr = curr->next;
    }

    return -1;
}

struct e820_table *vm_e820_table_get(vm_t *vm)
{
    struct e820_table *table = malloc(sizeof(struct e820_table));

    if (table == NULL)
    {
        return NULL;
    }

    table->length = linked_list_size(vm->memory_list);
    table->entries = malloc(sizeof(struct e820_entry) * table->length);

    if (table->entries == NULL)
    {
        free(table);
        return NULL;
    }

    struct linked_list_elt *current = vm->memory_list->head;

    for (size_t i = 0; current != NULL && i < table->length; ++i)
    {
        struct memory_mapped *map = (struct memory_mapped *)current->value;

        table->entries[i].base_address = map->guest_phys;
        table->entries[i].size = map->size;
        table->entries[i].type = E820_USABLE;

        current = current->next;
    }

    return table;
}

void vm_e820_table_free(struct e820_table *table)
{
    if (table == NULL)
    {
        return;
    }

    free(table->entries);
    free(table);
}

#include <stdio.h>

s32 vm_run(vm_t *vm)
{
    if (vm == NULL || vm->kvm_run == NULL)
    {
        return 0;
    }

    for (;;)
    {
        // Run again the VM at each VM exit
        if (ioctl(vm->vcpu_fd, KVM_RUN, 0) == -1)
        {
            printf("Failed to run\n");
            return 0;
        }

        // For now on ly check IO exit
        switch (vm->kvm_run->exit_reason)
        {
        case KVM_EXIT_IO: {
            if (vm->kvm_run->io.direction == KVM_EXIT_IO_OUT)
            {
                u8 *tmp = (u8 *)vm->kvm_run;

                if (vm->kvm_run->io.size == 1)
                {
                    u8 data = *(tmp + vm->kvm_run->io.data_offset);
                    io_handle_outb(vm->kvm_run->io.port, data);
                }
            }
            else if (vm->kvm_run->io.direction == KVM_EXIT_IO_IN)
            {
                u8 *tmp = (u8 *)vm->kvm_run;

                if (vm->kvm_run->io.size == 1)
                {
                    *(tmp + vm->kvm_run->io.data_offset) =
                        io_handle_inb(vm->kvm_run->io.port);
                }
            }
            break;
        }
        case KVM_EXIT_MMIO: {
            if (vm->kvm_run->mmio.is_write)
            {
                mmio_handle_write(vm->kvm_run->mmio.phys_addr,
                                  vm->kvm_run->mmio.data,
                                  vm->kvm_run->mmio.len);
            }
            break;
        }
        case KVM_EXIT_HLT: {
            printf("HALT instruction dumping regs\n");

            vm_dump_regs(vm);
            sleep(2);
            break;
        }
        default:
            printf("Unknown vm exit %d\n", vm->kvm_run->exit_reason);
        }
    }

    return 1;
}

s32 vm_dump_regs(vm_t *vm)
{
    struct kvm_regs regs = { 0 };

    if (ioctl(vm->vcpu_fd, KVM_GET_REGS, &regs) == -1)
    {
        return 0;
    }

    printf("rax: %llx\n", regs.rax);
    printf("rbx: %llx\n", regs.rbx);
    printf("rcx: %llx\n", regs.rcx);
    printf("rdx: %llx\n", regs.rdx);
    printf("rsi: %llx\n", regs.rsi);
    printf("rsp: %llx\n", regs.rsp);
    printf("r8: %llx\n", regs.r8);
    printf("r9: %llx\n", regs.r9);
    printf("r10: %llx\n", regs.r10);
    printf("r11: %llx\n", regs.r11);
    printf("r12: %llx\n", regs.r12);
    printf("r13: %llx\n", regs.r13);
    printf("r14: %llx\n", regs.r14);
    printf("r15: %llx\n", regs.r15);
    printf("rip: %llx\n", regs.rip);
    printf("rflags: %llx\n", regs.rflags);

    return 1;
}
