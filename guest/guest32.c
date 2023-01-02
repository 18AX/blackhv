#define STACK_SIZE 16384

#define COM1 0x3F8

static char stack[STACK_SIZE];

static void outb(unsigned short addr, unsigned char value)
{
    asm volatile("outb %0,%1" : : "a"(value), "d"(addr));
}

__attribute__((naked, section(".boot"))) void _start(void)
{
    asm volatile("mov %0,%%esp\n"
                 "mov %0,%%ebp\n"
                 :
                 : "r"(stack + STACK_SIZE));

    outb(COM1, 'H');
    outb(COM1, 'e');
    outb(COM1, 'l');
    outb(COM1, 'l');
    outb(COM1, 'o');
    outb(COM1, ' ');
    outb(COM1, 'W');
    outb(COM1, 'o');
    outb(COM1, 'r');
    outb(COM1, 'l');
    outb(COM1, 'd');
    outb(COM1, '\n');

    for (;;)
    {
        asm volatile("hlt");
    }
}
