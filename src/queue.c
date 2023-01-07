#include <blackhv/queue.h>
#include <pthread.h>
#include <stdlib.h>

queue_t *queue_new(size_t buffer_size)
{
    queue_t *queue = malloc(sizeof(queue_t));

    if (queue == NULL)
    {
        return NULL;
    }

    queue->buffer = malloc(buffer_size);

    if (queue->buffer == NULL)
    {
        free(queue);
        return queue;
    }

    queue->buffer_size = buffer_size;
    queue->front = 0;
    queue->rear = 0;
    queue->is_full = 0;
    pthread_mutex_init(&queue->mutex, NULL);

    return queue;
}

size_t queue_write(queue_t *queue, u8 *buffer, size_t size)
{
    if (queue == NULL || size == 0 || queue->is_full == 1)
    {
        return 0;
    }

    pthread_mutex_lock(&queue->mutex);

    size_t i = 0;
    do
    {
        queue->buffer[queue->rear] = buffer[i];
        queue->rear = (queue->rear + 1) % queue->buffer_size;

        ++i;
    } while (queue->front != queue->rear && i < size);

    if (queue->front == queue->rear)
    {
        queue->is_full = 1;
    }

    pthread_mutex_unlock(&queue->mutex);

    return i;
}

size_t queue_read(queue_t *queue, u8 *buffer, size_t size)
{
    if (queue == NULL || size == 0
        || (queue->is_full == 0 && queue->front == queue->rear))
    {
        return 0;
    }

    pthread_mutex_lock(&queue->mutex);

    size_t i = 0;

    do
    {
        buffer[i] = queue->buffer[queue->front];
        queue->front = (queue->front + 1) % queue->buffer_size;

        ++i;
    } while (queue->front != queue->rear && i < size);

    if (i != 0)
    {
        queue->is_full = 0;
    }

    pthread_mutex_unlock(&queue->mutex);

    return i;
}

u32 queue_empty(queue_t *queue)
{
    if (queue == NULL)
    {
        return 0;
    }

    return queue->is_full != 1 && queue->front == queue->rear;
}

void queue_destroy(queue_t *queue)
{
    if (queue == NULL)
    {
        return;
    }

    free(queue->buffer);
    free(queue);
}