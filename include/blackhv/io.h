#ifndef IO_HEADER
#define IO_HEADER

#include <blackhv/types.h>

struct handler
{
    void *params;
    void (*handler)(u16, u8, void *);
};

void io_set_outb_handler(u16 port,
                         void (*handler)(u16 port, u8 data, void *params),
                         void *params);

void io_handle_outb(u16 port, u8 data);

#endif