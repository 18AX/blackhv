#include <blackhv/io.h>
#include <blackhv/serial.h>
#include <err.h>
#include <stdlib.h>

// Registers
#define THR 0x0 // Transmitter Holding Buffer, write
#define RBR 0x0 // Receiver buffer, read
#define DLL 0x0 // Divisor Latch low byte, dlab 1, read and write
#define IER 0x1 // Interrupt enable register, read and write
#define DLH 0x1 // Divisor Latch high byte, dlab 1, read and write
#define IIR 0x2 // Interrupt Identification register, read
#define FCR 0x2 // FIFO Control register, write
#define LCR 0x3 // Line Control register, read and write
#define MCR 0x4 // Modem Control register, read and write
#define LSR 0x5 // Line status register, read
#define MSR 0x6 // Modem status register, read
#define SR 0x7 // Scratch Register, read and write

static void write_data(serial_t *serial, u8 data)
{
    queue_write(serial->guest_queue, &data, 1);
}

#include <stdio.h>
static void serial_outb(u16 port, u8 data, void *params)
{
    if (params == NULL)
    {
        errx(1, "serial: params cannot be null");
    }

    serial_t *serial = (serial_t *)params;

    u16 serial_register = port - serial->port;

    switch (serial_register)
    {
    case THR:
        write_data(serial, data);
        break;
    default:
        fprintf(stderr, "serial: register not supported yet %x", port);
        break;
    }
}

static u8 serial_inb(u16 port, void *params)
{
    if (params == NULL)
    {
        errx(1, "serial: params cannot be null");
    }

    serial_t *serial = (serial_t *)params;
    u16 serial_port = port - serial->port;

    switch (serial_port)
    {
    case RBR: {
        u8 data = 0;

        queue_read(serial->host_queue, &data, 1);

        return data;
    }
    case LSR: {
        u8 line_status = 0x0;

        // Data ready and serial ready to receive more data
        line_status |=
            ((!queue_empty(serial->host_queue)) & 0x1) | (0x1 << 0x5);

        return line_status;
    }
        fprintf(stderr, "serial: register not supperted yet %x", port);
    }

    // Unreachable
    return 0x0;
}

serial_t *serial_new(u16 port, size_t internal_buffer_size)
{
    serial_t *serial = malloc(sizeof(serial_t));

    if (serial == NULL)
    {
        return NULL;
    }

    serial->port = port;
    serial->guest_queue = queue_new(internal_buffer_size);
    serial->host_queue = queue_new(internal_buffer_size);

    if (serial->guest_queue == NULL)
    {
        free(serial);
        return NULL;
    }

    if (serial->host_queue == NULL)
    {
        queue_destroy(serial->guest_queue);
        free(serial);
        return NULL;
    }

    struct handler handler = { .inb_handler = serial_inb,
                               .outb_handler = serial_outb,
                               .params = serial };

    // Register the handler for all the serial register
    for (size_t i = 0; i < 8; ++i)
    {
        io_register_handler(port + i, handler);
    }

    return serial;
}

void serial_destroy(serial_t *serial)
{
    if (serial == NULL)
    {
        return;
    }

    // Unregister the handler for all the serial register
    for (size_t i = 0; i < 8; ++i)
    {
        io_unregister_handler(serial->port + i);
    }

    queue_destroy(serial->guest_queue);
    queue_destroy(serial->host_queue);
    free(serial);
}

size_t serial_read(serial_t *serial, u8 *buffer, size_t len)
{
    if (serial == NULL || buffer == NULL)
    {
        return -1;
    }

    return queue_read(serial->guest_queue, buffer, len);
}

size_t serial_write(serial_t *serial, u8 *buffer, size_t len)
{
    if (serial == NULL || buffer == NULL)
    {
        return -1;
    }

    return queue_write(serial->host_queue, buffer, len);
}
