#include "tgmj_handler.h"

#include "tgmtracker.h"
#include "tgm_memorymap.h"

#include "emu.h"
#include "debug/express.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/mman.h>
#include <fcntl.h>

const offs_t LEVEL_ADDR = 0x0017699A;
const offs_t TIMER_ADDR = 0x0017698C;

static const address_space* space = NULL;
struct tgmj_state curState = {0};

void readState(const address_space* space, struct tgmj_state* state)
{
    state->level = memory_read_word(space, LEVEL_ADDR);
    state->timer = memory_read_word(space, TIMER_ADDR);
}

void tgmj_setAddressSpace(running_machine* machine)
{
    space = tt_setAddressSpace(machine);
}

void tgmj_create_mmap()
{
    tgm_mm_create(sizeof(struct tgmj_state));
}

void tgmj_destroy_mmap()
{
    tgm_mm_destroy();
}

void tgmj_run(bool fumen, bool tracker)
{
    if (tracker)
    {
        readState(space, &curState);

        // Write current state to memory map
        *(struct tgmj_state*)tgm_mm_getMapPointer() = curState;
    }
}
