#include "tgmtracker.h"

#include "tgmj_handler.h"
#include "tgm2p_handler.h"

struct tgmtracker_t tgm2p_tracker =
{
    .setAddressSpace = tgm2p_setAddressSpace,
    .initialize      = tgm2p_create_mmap,
    .cleanup         = tgm2p_destroy_mmap,
    .run             = tgm2p_run
};

struct tgmtracker_t tgmj_tracker =
{
    .setAddressSpace = tgmj_setAddressSpace,
    .initialize      = tgmj_create_mmap,
    .cleanup         = tgmj_destroy_mmap,
    .run             = tgmj_run
};
