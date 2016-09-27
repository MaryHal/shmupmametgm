#include "tgmj_handler.h"

#include "tgmtracker.h"
#include "tgm_memorymap.h"

#include "tgm_common.h"

#include "emu.h"
#include "debug/express.h"

static const int GRADE_COUNT = 19;
static const char* GRADE_DISPLAY[GRADE_COUNT] =
{
    "9", "8", "7", "6", "5", "4", "3", "2", "1",
    "S1", "S2", "S3", "S4", "S5", "S6", "S7", "S8", "S9", "GM"
};

enum tgmj_internal_state
{
    TGMJ_IDLE         = 0,  // No game has started, just waiting...
    TGMJ_ACTIVE       = 20,
    TGMJ_LOCKING      = 30, // Cannot be influenced anymore
    TGMJ_LOCKED       = 60,
    TGMJ_LINECLEAR    = 40,

    TGMJ_ENTRY        = 10,
    TGMJ_ENTRY2       = 100,

    TGMJ_FADING1      = 111, // Blocks greying out at when topping out
    TGMJ_FADING2      = 112, // Blocks greying out at when topping out
    TGMJ_NAME_ENTRY   = 114,
    TGMJ_GAMEOVER     = 116,  // "Game Over" is being shown on screen.

    TGMJ_READY0       = 90, // READY!
    TGMJ_READY1       = 91, // READY!
    TGMJ_READY2       = 92, // READY!
    TGMJ_READY3       = 93, // READY!
    TGMJ_READY4       = 94, // READY!
    TGMJ_READY5       = 95, // GO!
    TGMJ_READY6       = 96, // GO!
};

#define MODE_20G_MASK  (1 << 0)
#define MODE_BIG_MASK  (1 << 2)
#define MODE_UKI_MASK  (1 << 3)
#define MODE_REV_MASK  (1 << 4)
#define MODE_MONO_MASK (1 << 5)
#define MODE_TLS_MASK  (1 << 7)

static void getModeName(char* buffer, size_t bufferLength, uint8_t gameMode)
{
    snprintf(buffer, bufferLength, "%s%s%s%s%s%s",
             gameMode & MODE_20G_MASK  ? "20G " : "",
             gameMode & MODE_BIG_MASK  ? "Big " : "",
             gameMode & MODE_UKI_MASK  ? "Uki " : "",
             gameMode & MODE_REV_MASK  ? "Reverse " : "",
             gameMode & MODE_MONO_MASK ? "Mono " : "",
             gameMode & MODE_TLS_MASK  ? "TLS" : "");
}

static bool inPlayingState(uint8_t state)
{
    return
        state == TGMJ_ACTIVE    ||
        state == TGMJ_LOCKING   ||
        state == TGMJ_LOCKED    ||
        state == TGMJ_LINECLEAR ||
        state == TGMJ_ENTRY     ||
        state == TGMJ_ENTRY2;
}

static const offs_t STATE_ADDR = 0x0017695D; // p1 State
static const offs_t LEVEL_ADDR = 0x0017699A;
static const offs_t TIMER_ADDR = 0x0017698C;

static const offs_t GRADE_ADDR       = 0x0017699C;  // Master-mode internal grade
static const offs_t GRADEPOINTS_ADDR = 0x00000000;  // Master-mode internal grade points
static const offs_t MROLLFLAGS_ADDR  = 0x00000000;  // M-Roll flags
static const offs_t INROLL_ADDR      = 0x00000000;  // p1 in-credit-roll
static const offs_t SECTION_ADDR     = 0x0017699E;  // p1 section index

static const offs_t TETRO_ADDR       = 0x001769D4;  // Current block
static const offs_t NEXT_ADDR        = 0x001769D2;  // Next block
static const offs_t CURRX_ADDR       = 0x001769DE;  // Current block X position
static const offs_t CURRY_ADDR       = 0x001769DA;  // Current block Y position
static const offs_t ROTATION_ADDR    = 0x001769D7;  // Current block rotation state

static const offs_t GAMEMODE_ADDR    = 0x001C005E;  // Current game mode

static const address_space* space = NULL;
static struct tgm_state curState = {0};

static void readState(const address_space* space, struct tgm_state* state)
{
    state->state     = memory_read_byte(space, STATE_ADDR);
    state->level     = memory_read_word(space, LEVEL_ADDR);
    state->timer     = memory_read_word(space, TIMER_ADDR);

    state->grade     = memory_read_word(space, GRADE_ADDR);

    state->tetromino = memory_read_byte(space, TETRO_ADDR);
    state->xcoord    = memory_read_word(space, CURRX_ADDR);
    state->ycoord    = memory_read_word(space, CURRY_ADDR);
    state->rotation  = memory_read_word(space, ROTATION_ADDR);

    state->gameMode  = memory_read_byte(space, GAMEMODE_ADDR);
}

void tgmj_setAddressSpace(running_machine* machine)
{
    space = tt_setAddressSpace(machine);
}

void tgmj_create_mmap()
{
    tgm_mm_create(sizeof(struct tgm_state));
}

void tgmj_destroy_mmap()
{
    tgm_mm_destroy();
}

void tgmj_run(bool fumen, bool tracker)
{
    readState(space, &curState);

    if (tracker)
    {
        // Write current state to memory map
        struct tgm_state* mmapPtr = (struct tgm_state*)tgm_mm_getMapPointer();

	if (mmapPtr)
            *mmapPtr = curState;
    }
}
