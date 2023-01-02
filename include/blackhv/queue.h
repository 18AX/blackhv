#ifndef QUEUE_HEADER
#define QUEUE_HEADER

#include <blackhv/types.h>
#include <pthread.h>

typedef struct {
    pthread_mutex_t mutex;
    size_t buffer_size;
    u8 *buffer;
} queue_t;

#endif