#ifndef SCREEN_HEADER
#define SCREEN_HEADER

#include <blackhv/types.h>

typedef struct vm vm_t;
typedef struct screen screen_t;

s64 screen_init(vm_t *vm, u64 framebuffer_phys);
void *screen_run(void *params);
void screen_uninit(vm_t *vm);

#endif
