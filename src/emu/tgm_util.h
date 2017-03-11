#ifndef TGM_UTIL_H
#define TGM_UTIL_H

#include <stdlib.h>

void memorymap_create(size_t dataSize);
void memorymap_destroy();
void* memorymap_getPointer();

int createDir(const char* path);

#endif /* TGM_UTIL_H */
