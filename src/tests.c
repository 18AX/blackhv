#include <blackhv/queue.h>
#include <criterion/criterion.h>

Test(queue, queue_create)
{
    queue_t *q = queue_new(1024);

    cr_assert_neq(q, NULL);
    cr_assert_eq(q->buffer_size, 1024);
    cr_assert_eq(q->is_full, 0);
    cr_assert_eq(q->front, 0);
    cr_assert_eq(q->rear, 0);
}

Test(queue, queue_add_pop)
{
    queue_t *q = queue_new(512);

    char value = 'A';

    cr_assert_eq(queue_write(q, (unsigned char *)&value, sizeof(char)), 1);

    char readed = 'A';

    cr_assert_eq(queue_read(q, (unsigned char *)&readed, sizeof(char)), 1);

    cr_assert_eq(value, readed);
    cr_assert_eq(queue_empty(q), 1);
    cr_assert_eq(queue_read(q, (unsigned char *)&readed, sizeof(char)), 0);
}

Test(queue, queue_add_multiple)
{
    queue_t *q = queue_new(512);

    for (unsigned char i = 0; i < 16; ++i)
    {
        cr_assert_eq(queue_write(q, &i, sizeof(unsigned char)), 1);
    }

    for (unsigned char i = 0; i < 16; ++i)
    {
        unsigned char readed = 'B';

        cr_assert_eq(queue_read(q, &readed, sizeof(unsigned char)), 1);

        cr_assert_eq(i, readed);
    }

    unsigned char r = 0;

    cr_assert_eq(queue_empty(q), 1);
    cr_assert_eq(queue_read(q, &r, sizeof(char)), 0);
}