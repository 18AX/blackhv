# BlackHV

## vm.h

The main component for interacting with a virtual machine. The object `vm_t` is an instance of a virtual machine.

- [vm_new](#vm_new)
- [vm_destroy](#vm_destroy)
- [vm_alloc_memory](#vm_alloc_memory)
- [vm_memory_write](#vm_memory_write)
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

### vm_alloc_memory

```c
s32 vm_alloc_memory(vm_t *vm, u64 phys_addr, u64 size);
```

Allocate the memory that will be usable by the virtual machine. It is possible to allocate multiple memory area at different physical address.

**return**: 0 on error, 1 otherwise.

#### Example

```c
vm_t *vm = vm_new();

if (vm_alloc_memory(vm, 0x0, MB_1) == 0)
{
    errx(1, "Failed to allocate 1 Mb of memory");
}
```

### vm_memory_write

```c
s64 vm_memory_write(vm_t *vm, u64 destination, u8 *buffer, u64 size);
```

Write data to the guest memory. The destination is a physical memory address.

**return**: the number of bytes written.

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