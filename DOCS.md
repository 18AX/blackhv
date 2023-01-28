# BlackHV

## vm.h

The main component for interacting with a virtual machine. The object `vm_t` is an instance of a virtual machine.

- [vm_new](#vm_new)
- [vm_destroy](#vm_destroy)
- [vm_vcpu_set_state](#vm_vcpu_set_state)
- [vm_run](#vm_run)

### vm_new

```c
vm_t *vm_new(void);
```

Create a new `vm_t` object. The object should be destroy using the function `vm_destroy`.

**return**: `vm_t*` object on success, `NULL` otherwise.

### vm_destroy

```c
void vm_destroy(vm_t *vm);
```

Destroy and free all the resources related to the vm object.

### vm_vcpu_init_state

```c
s32 vm_vcpu_init_state(vm_t *vm,
                       u64 code_addr,
                       u64 tss_address,
                       u64 identity_map_address,
                       u32 flags);
```

Initialize the state of a virtual CPU. The `code_addr` is the address set in the instruction pointer register. The `tss_address` **must be not** conflicting with a mmio memory area or a memory slot. Same for `identity_map_address`.

The list of possiblie flags:
- REAL_MODE
- PROTECTED_MODE
- CREATE_IRQCHIP
- CREATE_PIT

**return**: 1 on success, 0 otherwise.

#### Example

```c
if (vm_vcpu_set_state(vm, START_ADDRESS, PROTECTED_MODE) != 1)
{
    errx(1, "Failed to initialize virtual cpu");
}
```

### vm_run

```c
s32 vm_run(vm_t *vm);
```

Run the virtual machine, blocking call.

**return**: 1 on success, 0 otherwise.

#### Example

```c
// The initialization of the vm_t object has been made before
if (vm_run(vm) != 1)
{
    errx(1, "Failed to run VM\n");
}
```

## memory.h

This header provides some functions to manage virtual machine memory.

- [memory_alloc](#memory_alloc)
- [memory_read](#memory_read)
- [memory_write](#memory_write)
- [e820_table_get](#e820_table_get)
- [e820_table_free](#e820_table_free)

### memory_alloc

```c
#define MEMORY_USABLE 0x1
#define MEMORY_MMIO 0x2

s32 memory_alloc(vm_t *vm, u64 phys_addr, u64 size, u32 type);
```

Allocate the memory that will be usable by the virtual machine.

**return**: 0 on error, 1 otherwise.

#### Example

```c
vm_t *vm = vm_new();

if (memory_alloc(vm, 0x0, MB_1, MEMORY_USABLE) == 0)
{
    errx(1, "Failed to allocate 1 Mb of memory");
}
```

### memory_write

```c
s64 memory_write(vm_t *vm, u64 destination, u8 *buffer, u64 size);
```

Write data to the guest memory. The destination is a physical memory address.

**return**: the number of bytes written.

### memory_read

```c
s64 memory_read(vm_t *vm, u64 src_phys, u8 *buffer, u64 size);
```

Read data from the guest memory. The source is a physical memory address.

**return**: the number of bytes read.

### e820_table_get

```c
struct e820_table *vm_e820_table_get(vm_t *vm);
```

Get the e820 table (memory map) of a `vm_t` object

**return**: `struct e820_table *` on success, NULL otherwise. The return value has to be freed with `e820_table_free`

```c
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
```

#### Example

```c
struct e820_table *e820_table = e820_table_get(vm);

if (e820_table == NULL)
{
    return;
}

printf("e820 table:\n");
for (size_t i = 0; i < e820_table->length; ++i)
{
    printf("base address: %llx size: %llx type: %u\n",
           e820_table->entries[i].base_address,
           e820_table->entries[i].size,
           e820_table->entries[i].type);
}

printf("\n");

e820_table_free(e820_table);
```

### e820_table_free

```c
void e820_table_free(struct e820_table *table);
```

Free a `struct e820_table`.

## serial.h

This header provides some functions to emulate a UART device.

- [serial_new](#serial_new)
- [serial_destroy](#serial_destroy)
- [serial_read](#serial_read)
- [serial_write](#serial_write)

### serial_new

```c
serial_t *serial_new(u16 port, size_t internal_buffer_size);
```

Create a UART device on a defined port. Ports address to `port + 7` should not be conflicting with another IO device.

**return**: `serial_t` object on success, `NULL` otherwise.

#### Example

```c
// COM1 is define in serial.h
serial_t *serial = serial_new(COM1, 1024);

if (serial == NULL)
{
    errx(1, "Failed to initialize a serial device");
}
```

### serial_destroy

```c
void serial_destroy(serial_t *serial);
```

Free all the memory used by a `serial_t` object.

### serial_read

```c
size_t serial_read(serial_t *serial, u8 *buffer, size_t len);
```

Read data written by the guest on the port address.

**return**: number of bytes readed.

### serial_write

```c
size_t serial_write(serial_t *serial, u8 *buffer, size_t len);
```

Write data to the guest.

**return**: number of bytes written.

## mmio.h

Create a mmio handler to helps registering your mmio device emulation. `mmio_init` should be call before registering handlers.

```c
static void mmio_write_handler(struct mmio_region *region,
                        u64 address,
                        u8 data[8],
                        u32 len,
                        void *arg)
{
    (void)arg;
    printf(
        "mmio_id: %d address: %llx len: %u data: ", region->id, address, len);

    for (u32 i = 0; i < len; ++i)
    {
        printf("%x ", data[i]);
    }

    printf("\n");
}


void main(void)
{

    struct mmio_region region = { .base_address = 0xC0000000,
                                  .high_address = 0xC1000000,
                                  .write_handler = mmio_write_handler,
                                  .read_handler = NULL,
                                  .data = NULL };

    s32 mmio_region_id = mmio_register(&region);

    if (mmio_region_id < 0)
    {
        errx(1, "Failed to register a mmio region");
    }
}
```

### mmio_init

```c
void mmio_init(void);
```

Initialize the mmio handler. It needs to be done before registering handlers.

### mmio_register

```c
s32 mmio_register(struct mmio_region *region);
```

Register a MMIO region. The structure `mmio_region` describes a MMIO region and the write and read handler associated with it.

**return**: the ID of the memory region. The ID has to be used to unregister the region. On error it returns -1.

```c
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
    void *data; // Data given as a arg to the write and read handler
};
```

### mmio_unregister

```c
void mmio_unregister(s32 id);
```

Unregister a MMIO region. The ID corresponds to the ID given by `mmio_register`.

## screen.h

This header provides some functions to emulate a screen.

- [screen_init](#screen_init)
- [screen_run](#screen_run)
- [screen_uninit](#screen_uninit)

### screen_init

```c
s64 screen_init(vm_t *vm, u64 framebuffer_phys);
```

Initialize the screen component of a VM.
`framebuffer_phys` contains the desired address for the framebuffer.
A guest memory space will be allocated of size `FB_WIDTH * FB_HEIGHT * FB_BPP` (refer to *framebuffer.h*).

Return `1` on success, `0` otherwise.

### screen_run

```c
void *screen_run(void *params);
```

Blocking loop to handle screen events and refreshs. Should be used as a thread worker.
`params` must be a valid `vm_t` pointer.

### screen_uninit

```c
void screen_uninit(vm_t *vm);
```

Free memory used by a VM screen.
