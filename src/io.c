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

s32 io_handle_outb(u16 port, u8 data)
{
    if (handlers[port].outb_handler != NULL)
    {
        handlers[port].outb_handler(port, data, handlers[port].params);
        return 1;
    }

    return 0;
}

s32 io_handle_inb(u16 port, u8 *output)
{
    if (handlers[port].inb_handler != NULL)
    {
        *output = handlers[port].inb_handler(port, handlers[port].params);
        return 1;
    }

    // TODO: handle this correctly
    return 0;
}

s32 io_handle_outw(u16 port, u16 data)
{
    if (handlers[port].outw_handler != NULL)
    {
        handlers[port].outw_handler(port, data, handlers[port].params);
        return 1;
    }

    return 0;
}

s32 io_handle_inw(u16 port, u16 *output)
{
    if (handlers[port].inw_handler != NULL)
    {
        *output = handlers[port].inw_handler(port, handlers[port].params);
        return 1;
    }

    // TODO: handle this correctly
    return 0;
}
