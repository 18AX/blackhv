#include <blackhv/cpu.h>
#include <blackhv/io.h>
#include <blackhv/mmio.h>
#include <blackhv/vm.h>
#include <err.h>
#include <fcntl.h>
#include <linux/kvm.h>
#include <linux/kvm_para.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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
    vm->mem = memory_new();
    vm->screen = NULL;

    if (vm->mem == NULL)
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

    memory_destroy(vm->mem);
    close(vm->kvm_fd);
    close(vm->vm_fd);
    free(vm);
}

static s32 set_real_mode(vm_t *vm)
{
    struct kvm_sregs sregs;
    if (ioctl(vm->vcpu_fd, KVM_GET_SREGS, &sregs) < 0)
    {
        return -1;
    }

    sregs.cs.selector = 0;
    sregs.cs.base = 0;

    return 0;
}

static s32 set_protected_mode(vm_t *vm)
{
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

    return ioctl(vm->vcpu_fd, KVM_SET_SREGS, &sregs) == 0;
}

s32 vm_set_regs(vm_t *vm, struct kvm_regs *regs)
{
    return regs != NULL && ioctl(vm->vcpu_fd, KVM_SET_REGS, regs) == 0;
}

s32 vm_get_regs(vm_t *vm, struct kvm_regs *regs)
{
    return regs != NULL && ioctl(vm->vcpu_fd, KVM_GET_REGS, regs) == 0;
}

struct mycpuid
{
    u32 nent;
    u32 padding;
    struct kvm_cpuid_entry2 entries[128];
};

static s32 setup_cpuid(vm_t *vm)
{
    struct mycpuid cpuid = { .nent = 128, .padding = 0 };

    if (ioctl(vm->kvm_fd, KVM_GET_SUPPORTED_CPUID, &cpuid) != 0)
    {
        return 0;
    }

    for (size_t i = 0; i < cpuid.nent; ++i)
    {
        if (cpuid.entries[i].function == KVM_CPUID_SIGNATURE)
        {
            cpuid.entries[i].eax = KVM_CPUID_FEATURES;
            cpuid.entries[i].ebx = 0x4b4d564b;
            cpuid.entries[i].ecx = 0x564b4d56;
            cpuid.entries[i].edx = 0x4d;
        }
    }

    return ioctl(vm->vcpu_fd, KVM_SET_CPUID2, &cpuid);
}

s32 vm_vcpu_init_state(vm_t *vm,
                       u64 tss_address,
                       u64 identity_map_addr,
                       u32 mode)
{
    if (ioctl(vm->vm_fd, KVM_SET_TSS_ADDR, tss_address, 0) < 0
        || ioctl(vm->vm_fd, KVM_SET_IDENTITY_MAP_ADDR, &identity_map_addr, 0)
            < 0)
    {
        return 0;
    }

    if ((mode & CREATE_IRQCHIP) != 0
        && ioctl(vm->vm_fd, KVM_CREATE_IRQCHIP, 0) < 0)
    {
        return 0;
    }

    if ((mode & CREATE_PIT) != 0)
    {
        struct kvm_pit_config pit_config = { .flags = 0 };

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

    if (setup_cpuid(vm) < 0)
    {
        return 0;
    }

    if ((mode & REAL_MODE) != 0)
    {
        return set_real_mode(vm);
    }
    else if ((mode & PROTECTED_MODE) != 0)
    {
        return set_protected_mode(vm);
    }

    return 1;
}

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
            fprintf(stderr, "Failed to VM\n");
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
                    if (io_handle_inb(vm->kvm_run->io.port,
                                      tmp + vm->kvm_run->io.data_offset)
                        == 0)
                    {
                        fprintf(stderr, "Not supported port\n");
                    }
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
            fprintf(stderr, "Unknown vm exit %d\n", vm->kvm_run->exit_reason);
            return 0;
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
