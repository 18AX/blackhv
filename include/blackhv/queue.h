#ifndef QUEUE_HEADER
#define QUEUE_HEADER

#include <blackhv/types.h>
#include <pthread.h>

typedef struct
{
    pthread_mutex_t mutex;
    size_t buffer_size;
    size_t front;
    size_t rear;
    u8 is_full;
    u8 *buffer;
} queue_t;

queue_t *queue_new(size_t buffer_size);

size_t queue_write(queue_t *queue, u8 *buffer, size_t size);

size_t queue_read(queue_t *queue, u8 *buffer, size_t size);

u32 queue_empty(queue_t *queue);

void queue_destroy(queue_t *queue);

#endif