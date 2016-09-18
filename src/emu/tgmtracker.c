#include "tgmtracker.h"

#include "tgmj_handler.h"
#include "tgm2p_handler.h"

#include "tgm_memorymap.h"

#include "emu.h"

const address_space* tt_setAddressSpace(running_machine* machine)
{
    // Search for maincpu
    for (running_device* device = machine->devicelist.first(); device != NULL; device = device->next)
    {
        if (mame_stricmp(device->tag(), "maincpu") == 0)
        {
            /* return device->space(ADDRESS_SPACE_PROGRAM + (0 - EXPSPACE_PROGRAM_LOGICAL)); */
            return device->space(ADDRESS_SPACE_PROGRAM);
        }
    }

    return NULL;
}

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
