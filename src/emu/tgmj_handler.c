#include "tgmj_handler.h"

#include "emu.h"
#include "debug/express.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/mman.h>
#include <fcntl.h>

static const char* sharedMemKey = "tgmj_data";
static int fd = 0;
static const size_t vSize = sizeof(struct tgmj_state);
static struct tap_state* sharedAddr = NULL;

void tgmj_create_mmap()
{
    fd = shm_open(sharedMemKey, O_RDWR | O_CREAT | O_TRUNC, S_IRWXO | S_IRWXG | S_IRWXU);

    // Stretch our new file to the suggested size.
    if (lseek(fd, vSize - 1, SEEK_SET) == -1)
    {
        perror("Could not stretch file via lseek");
    }

    // In order to change the size of the file, we need to actually write some
    // data. In this case, we'll be writing an empty string ('\0').
    if (write(fd, "", 1) != 1)
    {
        perror("Could not write the final byte in file");
    }

    sharedAddr = (struct tgmj_state*)mmap(NULL, vSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (sharedAddr == MAP_FAILED)
    {
        perror("Could not map memory");
    }

    if(mlock(sharedAddr, vSize) != 0)
    {
        perror("mlock failure");
    }
}

void tgmj_destroy_mmap()
{
    if (munlock(sharedAddr, vSize) != 0)
        perror("Error unlocking memory page");

    if (munmap(sharedAddr, vSize) != 0)
        perror("Error unmapping memory pointer");

    if (close(fd) != 0)
        perror("Error closing file");

    shm_unlink(sharedMemKey);
}

const offs_t LEVEL_ADDR = 0x0017699A;
const offs_t TIMER_ADDR = 0x0017698C;

void readState(const address_space* space, struct tgmj_state* state)
{
    state->level = memory_read_word(space, LEVEL_ADDR);
    state->timer = memory_read_word(space, TIMER_ADDR);
}

static const address_space* space = NULL;
struct tgmj_state curState = {0};

void tgmj_setAddressSpace(running_machine* machine)
{
    // Search for maincpu
    for (running_device* device = machine->devicelist.first(); device != NULL; device = device->next)
    {
        if (mame_stricmp(device->tag(), "maincpu") == 0)
        {
            space = device->space(ADDRESS_SPACE_PROGRAM + (0 - EXPSPACE_PROGRAM_LOGICAL));
            return;
        }
    }
}

void tgmj_run(bool fumen, bool tracker)
{
    if (tracker)
    {
        readState(space, &curState);

        // Write current state to memory map
        *sharedAddr = curState;
    }
}
