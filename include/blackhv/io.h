#ifndef IO_HEADER
#define IO_HEADER

#include <blackhv/types.h>

struct handler
{
    void *params;
    void (*outb_handler)(u16, u8, void *);
    void (*outw_handler)(u16, u16, void *);
    u8 (*inb_handler)(u16, void *);
    u16 (*inw_handler)(u16, void *);
};

void io_register_handler(u16 port, struct handler hdl);

void io_unregister_handler(u16 port);

s32 io_handle_outb(u16 port, u8 data);

s32 io_handle_inb(u16 port, u8 *output);

s32 io_handle_outw(u16 port, u16 data);

s32 io_handle_inw(u16 port, u16 *output);

#endif