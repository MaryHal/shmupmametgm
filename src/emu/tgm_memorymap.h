#ifndef TGM_MEMORYMAP_H
#define TGM_MEMORYMAP_H

#include <stdlib.h>

void tgm_mm_create(size_t dataSize);
void tgm_mm_destroy();

void* tgm_mm_getMapPointer();

#endif /* TGM_MEMORYMAP_H */
