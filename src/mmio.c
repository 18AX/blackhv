#include <blackhv/mmio.h>
#include <string.h>

#define MAX_MMIO_REGION_COUNT 64

static struct mmio_region mmio_regions[MAX_MMIO_REGION_COUNT] = { 0 };

void mmio_init()
{
    // We set the value -1 to identify available mmio regions
    for (s32 i = 0; i < MAX_MMIO_REGION_COUNT; ++i)
    {
        mmio_regions[i].id = -1;
    }
}

s32 mmio_register(struct mmio_region *region)
{
    for (s32 i = 0; i < MAX_MMIO_REGION_COUNT; ++i)
    {
        if (mmio_regions[i].id == -1)
        {
            memcpy(&mmio_regions[i], region, sizeof(struct mmio_region));
            mmio_regions[i].id = i;
            region->id = i;

            return i;
        }
    }

    // We did not find any available region
    return -1;
}

void mmio_unregister(s32 id)
{
    if (id < MAX_MMIO_REGION_COUNT)
    {
        mmio_regions[id].id = -1;
    }
}

void mmio_handle_write(u64 address, u8 data[8], u32 len)
{
    for (s32 i = 0; i < MAX_MMIO_REGION_COUNT; ++i)
    {
        if (mmio_regions[i].base_address <= address
            && mmio_regions[i].high_address > address
            && mmio_regions[i].write_handler != NULL)
        {
            mmio_regions[i].write_handler(
                &mmio_regions[i], address, data, len, mmio_regions[i].data);
        }
    }
}