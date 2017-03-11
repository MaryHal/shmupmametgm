#include "tgm_memorymap.h"
#include "tgm_util.h"

void tgm_mm_create(size_t dataSize)
{
    memorymap_create(dataSize);
}

void tgm_mm_destroy()
{
    memorymap_destroy();
}

void* tgm_mm_getMapPointer()
{
    return memorymap_getPointer();
}
