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