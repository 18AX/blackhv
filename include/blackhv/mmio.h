#ifndef MMIO_HEADER
#define MMIO_HEADER

#include <blackhv/types.h>
#include <blackhv/vm.h>

struct mmio_region
{
    s32 id; // ID will be set by the mmio_register function
    u64 base_address;
    u64 high_address;
    void (*write_handler)(struct mmio_region *region,
                          u64 address,
                          u8 data[8],
                          u32 len,
                          void *arg);
    void (*read_handler)(struct mmio_region *region,
                         u64 address,
                         u8 data[8],
                         u32 len,
                         void *arg);
    void *data;
};

void mmio_init(void);

s32 mmio_register(vm_t *vm, struct mmio_region *region);

void mmio_unregister(s32 id);

void mmio_handle_write(u64 address, u8 data[8], u32 len);

#endif