#include "tgmj_handler.h"

#include "tgmtracker.h"
#include "tgm_memorymap.h"

#include "emu.h"
#include "debug/express.h"

enum tgmj_internal_state
{
    TGMJ_IDLE         = 0,  // No game has started, just waiting...
    TGMJ_ACTIVE       = 20,
    TGMJ_LOCKING      = 30, // Cannot be influenced anymore
    TGMJ_LOCKED       = 60,
    TGMJ_LINECLEAR    = 40,
    TGMJ_GAMEOVER     = 116,  // "Game Over" is being shown on screen.

    TGMJ_ENTRY        = 10,
    TGMJ_ENTRY2       = 100,

    TGMJ_FADING1      = 111, // Blocks greying out at when topping out
    TGMJ_FADING2      = 112, // Blocks greying out at when topping out

    TGMJ_READY0       = 90, // READY!
    TGMJ_READY1       = 91, // READY!
    TGMJ_READY2       = 92, // READY!
    TGMJ_READY3       = 93, // READY!
    TGMJ_READY4       = 94, // READY!
    TGMJ_READY5       = 95, // GO!
    TGMJ_READY6       = 96, // GO!
};

static const offs_t STATE_ADDR = 0x0017695D; // p1 State
static const offs_t LEVEL_ADDR = 0x0017699A;
static const offs_t TIMER_ADDR = 0x0017698C;

static const offs_t GRADE_ADDR       = 0x0017699D;  // Master-mode internal grade
static const offs_t GRADEPOINTS_ADDR = 0x00000000;  // Master-mode internal grade points
static const offs_t MROLLFLAGS_ADDR  = 0x00000000;  // M-Roll flags
static const offs_t INROLL_ADDR      = 0x00000000;  // p1 in-credit-roll
static const offs_t SECTION_ADDR     = 0x00000000;  // p1 section index

static const offs_t TETRO_ADDR       = 0x001769D4;  // Current block
static const offs_t NEXT_ADDR        = 0x001769D2;  // Next block
static const offs_t CURRX_ADDR       = 0x001769DE;  // Current block X position
static const offs_t CURRY_ADDR       = 0x001769DA;  // Current block Y position
static const offs_t ROTATION_ADDR    = 0x001769D7;  // Current block rotation state

static const offs_t GAMEMODE_ADDR    = 0x00000000;  // Current game mode

static const address_space* space = NULL;
struct tgmj_state curState = {0};

void readState(const address_space* space, struct tgmj_state* state)
{
    state->state     = memory_read_byte(space, STATE_ADDR);
    state->level     = memory_read_word(space, LEVEL_ADDR);
    state->timer     = memory_read_word(space, TIMER_ADDR);

    state->grade     = memory_read_word(space, GRADE_ADDR);

    state->tetromino = memory_read_byte(space, TETRO_ADDR);
    state->xcoord    = memory_read_word(space, CURRX_ADDR);
    state->ycoord    = memory_read_word(space, CURRY_ADDR);
    state->rotation  = memory_read_word(space, ROTATION_ADDR);

    printf("%d %d %d %d %d %d %d %d\n", state->state, state->level, state->grade, state->tetromino, memory_read_byte(space, NEXT_ADDR), state->xcoord, state->ycoord, state->rotation);
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

        /* // Write current state to memory map */
        /* struct tgmj_state* mmapPtr = (struct tgmj_state*)tgm_mm_getMapPointer();  */

	/* if (mmapPtr) */
        /*     *mmapPtr = curState; */
    }
}
