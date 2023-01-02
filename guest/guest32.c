#define STACK_SIZE 16384

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

    outb(0xE9, 'H');
    outb(0xE9, 'e');
    outb(0xE9, 'l');
    outb(0xE9, 'l');
    outb(0xE9, 'o');
    outb(0xE9, ' ');
    outb(0xE9, 'W');
    outb(0xE9, 'o');
    outb(0xE9, 'r');
    outb(0xE9, 'l');
    outb(0xE9, 'd');
    outb(0xE9, '\n');

    for (;;)
    {
        asm volatile("hlt");
    }
}
