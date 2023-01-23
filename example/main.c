#include <err.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        errx(1, "invalid args: ./linux_example <bzimage>");
    }

    printf("bzimage %s\n", argv[1]);
    return 0;
}
