#include <blackhv/io.h>
#include <stddef.h>

static struct handler handlers[0xFFFF] = { 0x0 };

void io_set_outb_handler(u16 port,
                         void (*handler)(u16 port, u8 data, void *params),
                         void *params)
{
    handlers[port].handler = handler;
    handlers[port].params = params;
}

void io_handle_outb(u16 port, u8 data)
{
    if (handlers[port].handler != NULL)
    {
        handlers[port].handler(port, data, handlers[port].params);
    }
}