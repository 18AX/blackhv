#define STACK_SIZE 16384

#define COM1 0x3F8

static char stack[STACK_SIZE];

static void outb(unsigned short addr, unsigned char value)
{
    asm volatile("outb %0,%1" : : "a"(value), "d"(addr));
}

static unsigned char inb(unsigned short addr)
{
    unsigned char res;

    asm volatile("inb %1, %0" : "=&a"(res) : "d"(addr));

    return res;
}

void serial_write_char(char c)
{
    unsigned char status = inb(COM1 + 5);

    // Wait for the serial to be ready
    while (((status >> 5) & 1) == 0)
    {
        asm volatile("pause");
        status = inb(COM1 + 5);
    }

    outb(COM1, c);
}

void serial_write_str(char *str)
{
    for (int i = 0; str[i] != '\0'; ++i)
    {
        serial_write_char(str[i]);
    }
}

unsigned char serial_read()
{
    unsigned char status = inb(COM1 + 5);

    // Wait for some data to be available
    while ((status & 0x1) == 0)
    {
        asm volatile("pause");
        status = inb(COM1 + 5);
    }

    return inb(COM1);
}

__attribute__((naked, section(".boot"))) void _start(void)
{
    asm volatile("mov %0,%%esp\n"
                 "mov %0,%%ebp\n"
                 :
                 : "r"(stack + STACK_SIZE));

    serial_write_str("Echo program running in KVM\n");

    char *ptr = (char *)0xC000000A;
    *ptr = 'A';

    for (;;)
    {
        serial_write_char(serial_read());
    }
}
