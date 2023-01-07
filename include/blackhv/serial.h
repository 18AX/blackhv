#ifndef SERIAL_HEADER
#define SERIAL_HEADER

#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8

#include <blackhv/queue.h>
#include <blackhv/types.h>
#include <stddef.h>

typedef struct
{
    u16 port;
    queue_t *guest_queue; // Write from guest to host
    queue_t *host_queue; // Write from host to guest
} serial_t;

/**
 * Create a new serial on a specific port
 */
serial_t *serial_new(u16 port, size_t internal_buffer_size);

void serial_destroy(serial_t *serial);

/**
 * Read data sent by the guest
 */
size_t serial_read(serial_t *serial, u8 *buffer, size_t len);

/**
 * Write data to the guest
 */
size_t serial_write(serial_t *serial, u8 *buffer, size_t len);

#endif