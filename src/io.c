#include <blackhv/io.h>
#include <stddef.h>
#include <string.h>

static struct handler handlers[0xFFFF] = { 0x0 };

void io_register_handler(u16 port, struct handler hdl)
{
    handlers[port] = hdl;
}

void io_unregister_handler(u16 port)
{
    memset(&handlers[port], 0x0, sizeof(struct handler));
}

void io_handle_outb(u16 port, u8 data)
{
    if (handlers[port].outb_handler != NULL)
    {
        handlers[port].outb_handler(port, data, handlers[port].params);
    }
}

u8 io_handle_inb(u16 port)
{
    if (handlers[port].inb_handler != NULL)
    {
        return handlers[port].inb_handler(port, handlers[port].params);
    }

    // TODO: handle this correctly
    return 0xFF;
}