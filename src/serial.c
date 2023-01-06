#include <blackhv/io.h>
#include <blackhv/serial.h>
#include <stdlib.h>

// Registers
#define THR 0x0 // Transmitter Holding Buffer

static void write_data(serial_t *serial, u8 data)
{
    queue_write(serial->queue, &data, 1);
}

static void serial_outb(u16 port, u8 data, void *params)
{
    if (params == NULL)
    {
        return;
    }

    serial_t *serial = (serial_t *)params;

    u16 serial_register = port - serial->port;

    switch (serial_register)
    {
    case THR:
        write_data(serial, data);
        break;
    }
}

static u8 serial_inb(u16 port, void *params)
{
    (void)port;
    (void)params;
    return 'T';
}

serial_t *serial_new(u16 port, size_t internal_buffer_size)
{
    serial_t *serial = malloc(sizeof(serial_t));

    if (serial == NULL)
    {
        return NULL;
    }

    serial->port = port;
    serial->queue = queue_new(internal_buffer_size);

    if (serial->queue == NULL)
    {
        free(serial);
        return NULL;
    }

    struct handler handler = { .inb_handler = serial_inb,
                               .outb_handler = serial_outb,
                               .params = serial };

    io_register_handler(port, handler);

    return serial;
}

void serial_destroy(serial_t *serial)
{
    if (serial == NULL)
    {
        return;
    }

    io_unregister_handler(serial->port);
    queue_destroy(serial->queue);
    free(serial);
}

size_t serial_read(serial_t *serial, u8 *buffer, size_t len)
{
    if (serial == NULL || buffer == NULL)
    {
        return -1;
    }

    return queue_read(serial->queue, buffer, len);
}
