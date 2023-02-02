#ifndef ATAPI_H
#define ATAPI_H

#include <blackhv/vm.h>

void atapi_init(vm_t *vm, int disk_fd);

#endif
