#include <blackhv/serial.h>
#include <stdlib.h>
#include <io.h>

// Registers
#define THR 0x0 // Transmitter Holding Buffer 

static void write_data(serial_t *serial, u8 data)
{
    // Serial internal buffer full
    if (serial->buffer_size >= serial->elements_count)
    {
        // TODO: generate an exception
        return;
    }

    serial->buffer[serial->pos % serial->buffer_size] = data;
    serial->pos = serial->pos % serial->buffer_size + 1;
    serial->elements_count += 1;
}

static void serial_handler(u16 port, u8 data, void *params)
{
    if (params == NULL)
    {
        return;
    }

    serial_t *serial = (serial_t*) params;

    u16 serial_register = port - serial->port;

    switch (serial_register) {
        case THR:
            write_data(serial, data);
            break;
    }    
}

serial_t *serial_new(u16 port, size_t internal_buffer_size)
{
    serial_t *serial = malloc(sizeof(serial_t));

    if (serial == NULL)
    {
        return NULL;
    }

    serial->port = port;
    serial->pos = 0;
    serial->elements_count = 0;
    serial->buffer_size = internal_buffer_size;
    serial->buffer = malloc(internal_buffer_size);

    if (serial->buffer == NULL)
    {
        free(serial);
        return NULL;
    }

    io_set_outb_handler(port, serial_handler, serial);

    return serial;
}

void serial_destroy(serial_t *serial)
{
    if (serial == NULL)
    {
        return;
    }

    io_set_outb_handler(serial->port, NULL, NULL);
    free(serial);
}

ssize_t serial_read(serial_t *serial, u8 *buffer, size_t len)
{
    if (serial == NULL || buffer == NULL)
    {
        return -1;
    }

    ssize_t readed = 0;

    for (; readed < len && readed < serial->elements_count; ++readed)
    {
        buffer[readed] = serial->buffer[serial->pos];
    }
}
