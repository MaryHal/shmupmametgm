#include "tgmtracker.h"

#include "tgmj_handler.h"
#include "tgm2p_handler.h"

#include "tgm_memorymap.h"

#include "tgm_common.h"

#include "emu.h"

static void initialize()
{
    tgm_mm_create(sizeof(struct tgm_state));
}

static void cleanup()
{
    tgm_mm_destroy();
}

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
    .initialize      = initialize,
    .cleanup         = cleanup,
    .run             = tgm2p_run
};

struct tgmtracker_t tgmj_tracker =
{
    .setAddressSpace = tgmj_setAddressSpace,
    .initialize      = initialize,
    .cleanup         = cleanup,
    .run             = tgmj_run
};
