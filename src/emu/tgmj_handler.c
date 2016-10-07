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
    TGMJ_LINECLEAR    = 40,
    TGMJ_LINECLEAR2   = 50,
    TGMJ_LOCKED       = 60,

    TGMJ_ENTRY        = 10,
    TGMJ_ENTRY2       = 100,

    TGMJ_UNKNOWN      = 110, // Probably a death state
    TGMJ_FADING1      = 111, // Blocks greying out at when topping out
    TGMJ_FADING2      = 112, // Blocks greying out at when topping out
    TGMJ_NAME_ENTRY   = 114,
    TGMJ_DEAD         = 115,
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
    if (gameMode == 0)
    {
        snprintf(buffer, bufferLength, "Normal");
    }

    snprintf(buffer, bufferLength, "%s%s%s%s%s%s",
             gameMode & MODE_BIG_MASK  ? "Big"     : "",
             gameMode & MODE_UKI_MASK  ? "Uki"     : "",
             gameMode & MODE_REV_MASK  ? "Reverse" : "",
             gameMode & MODE_MONO_MASK ? "Mono"    : "",
             gameMode & MODE_TLS_MASK  ? "TLS"     : "",
             gameMode & MODE_20G_MASK  ? "20G"     : "");
}

static bool inPlayingState(uint8_t state)
{
    return state != TGMJ_IDLE;
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
static const offs_t CURRX_ADDR       = 0x001769DA;  // Current block X position
static const offs_t CURRY_ADDR       = 0x001769DE;  // Current block Y position
static const offs_t ROTATION_ADDR    = 0x001769D7;  // Current block rotation state

static const offs_t GAMEMODE_ADDR    = 0x001C005E;  // Current game mode

static struct tgm_state curState = {0}, prevState = {0};

static const size_t MAX_TGM_STATES = 1300; // What a nice number
static struct tgm_state stateList[MAX_TGM_STATES];
static size_t stateListSize = 0;

static int gameModeAtStart = 0;

static const address_space* space = NULL;

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

static void pushStateToList(struct tgm_state* list, size_t* listSize, struct tgm_state* state)
{
    list[*listSize] = *state;
    (*listSize)++;
}

// Triple into suicide
static const size_t demo01_length = 12;
static struct tgm_state demo01[] =
{
    { 2, 9, 0, 0,  0, 5, 4, 3,  1, 0, 0, 0 },
    { 2, 9, 0, 4,  0, 3, 4, 3,  0, 0, 0, 0 },
    { 2, 9, 0, 5,  0, 5, 4, 4,  0, 0, 0, 0 },
    { 2, 9, 0, 6,  0, 6, 4, 6,  0, 0, 0, 0 },
    { 2, 9, 0, 7,  0, 8, 4, 8,  0, 0, 0, 0 },
    { 2, 9, 0, 8,  0, 3, 4, 10, 0, 0, 0, 0 },
    { 2, 9, 0, 9,  0, 2, 4, 11, 0, 0, 0, 0 },
    { 2, 9, 0, 10, 0, 6, 4, 13, 0, 0, 0, 0 },
    { 2, 9, 0, 11, 0, 7, 4, 15, 0, 0, 0, 0 },
    { 2, 9, 0, 12, 0, 5, 4, 17, 0, 0, 0, 0 },
    { 2, 9, 0, 13, 0, 4, 4, 19, 0, 0, 0, 0 },
    { 2, 9, 0, 14, 0, 3, 4, 21, 0, 0, 0, 0 },
    { 2, 9, 0, 15, 0, 8, 4, 21, 0, 0, 0, 0 },
};

// Stairs: wallkick demo
static const size_t demo02_length = 3;
static struct tgm_state demo02[] =
{
    { 2, 9, 0, 70, 0, 8, 8, 3,  2, 0, 0, 0 },
    { 2, 9, 0, 72, 0, 5, 8, 3,  0, 0, 0, 0 },
    { 2, 9, 0, 75, 0, 7, 4, 21, 0, 0, 0, 0 },
};

// High gravity in messy field
static const size_t demo03_length = 5;
static struct tgm_state demo03[] =
{
    { 2, 9, 0, 140, 0, 4, 2, 3,  2, 0, 0, 0 },
    { 2, 9, 0, 141, 0, 5, 7, 3,  2, 0, 0, 0 },
    { 2, 9, 0, 143, 0, 8, 6, 4,  3, 0, 0, 0 },
    { 2, 9, 0, 144, 0, 6, 6, 7,  3, 0, 0, 0 },
    { 2, 9, 0, 145, 0, 4, 4, 21, 0, 0, 0, 0 },
};

// S3 Level Player
static const size_t demo04_length = 13;
static struct tgm_state demo04[] =
{
    { 2, 9, 0, 500, 0, 2, 8, 4,  1, 0, 0, 0 },
    { 2, 9, 0, 502, 0, 8, 3, 5,  3, 0, 0, 0 },
    { 2, 9, 0, 503, 0, 5, 8, 4,  1, 0, 0, 0 },
    { 2, 9, 0, 504, 0, 5, 0, 4,  1, 0, 0, 0 },
    { 2, 9, 0, 505, 0, 4, 2, 7,  0, 0, 0, 0 },
    { 2, 9, 0, 506, 0, 3, 4, 8,  2, 0, 0, 0 },
    { 2, 9, 0, 507, 0, 8, 0, 7,  1, 0, 0, 0 },
    { 2, 9, 0, 508, 0, 7, 3, 10, 0, 0, 0, 0 },
    { 2, 9, 0, 509, 0, 4, 1, 9,  1, 0, 0, 0 },
    { 2, 9, 0, 510, 0, 7, 3, 12, 0, 0, 0, 0 },
    { 2, 9, 0, 511, 0, 6, 2, 9,  3, 0, 0, 0 },
    { 2, 9, 0, 512, 0, 6, 6, 6,  1, 0, 0, 0 },
    { 2, 9, 0, 514, 0, 8, 8, 6,  2, 0, 0, 0 },
};

// GM Level Player
static const size_t demo05_length = 13;
static struct tgm_state demo05[] =
{
    { 2, 9, 0, 500, 0, 2, 8, 4, 1,  0, 0, 0 },
    { 2, 9, 0, 502, 0, 8, 3, 5, 3,  0, 0, 0 },
    { 2, 9, 0, 503, 0, 5, 0, 4, 1,  0, 0, 0 },
    { 2, 9, 0, 504, 0, 5, 8, 4, 1,  0, 0, 0 },
    { 2, 9, 0, 505, 0, 4, 7, 5, 1,  0, 0, 0 },
    { 2, 9, 0, 508, 0, 3, 4, 6, 0,  0, 0, 0 },
    { 2, 9, 0, 509, 0, 8, 2, 5, 1,  0, 0, 0 },
    { 2, 9, 0, 510, 0, 7, 0, 5, 0,  0, 0, 0 },
    { 2, 9, 0, 511, 0, 4, 1, 7, 0,  0, 0, 0 },
    { 2, 9, 0, 512, 0, 7, 8, 5, 0,  0, 0, 0 },
    { 2, 9, 0, 513, 0, 6, 7, 4, 3,  0, 0, 0 },
    { 2, 9, 0, 517, 0, 6, 8, 4, 2,  0, 0, 0 },
    { 2, 9, 0, 518, 0, 8, 5, 4, 3,  0, 0, 0 },
};

static const size_t demo06_length = 4;
static struct tgm_state demo06[] =
{
    { 2, 9, 0, 40, 0, 7, 4, 3,  1,  0, 0, 0 },
    { 2, 9, 0, 41, 0, 2, 4, 6,  3,  0, 0, 0 },
    { 2, 9, 0, 42, 0, 3, 4, 8,  1,  0, 0, 0 },
    { 2, 9, 0, 43, 0, 5, 4, 21, 1,  0, 0, 0 },
};

static bool isDemoState(struct tgm_state* stateList, size_t listSize)
{
    return
        testDemoState(stateList, listSize, demo01, demo01_length) ||
        testDemoState(stateList, listSize, demo02, demo02_length) ||
        testDemoState(stateList, listSize, demo03, demo03_length) ||
        testDemoState(stateList, listSize, demo04, demo04_length) ||
        testDemoState(stateList, listSize, demo05, demo05_length) ||
        testDemoState(stateList, listSize, demo06, demo06_length);
}

static void writePlacementLog()
{
    if (stateListSize == 0)
    {
        printf("State list is empty!\n");
        return;
    }
    else if (isDemoState(stateList, stateListSize))
    {
        printf("Demo state detected!\n");
        return;
    }

    // Push the killing piece. We must use the previous state
    // since, upon death, the game clears some data.
    pushStateToList(stateList, &stateListSize, &prevState);

    // Create fumen directory if it doesn't exist.
    createDir("fumen/");
    createDir("fumen/tgmj");

    char directory[32];
    char timebuf[32];
    char filename[80];

    // Create a directory for the day if it doesn't already exist.
    time_t rawTime;
    time(&rawTime);
    const struct tm* timeInfo = localtime(&rawTime);
    strftime(directory, 32, "fumen/tgmj/%Y-%m-%d", timeInfo);

    char modeName[32];
    getModeName(modeName, 32, gameModeAtStart);

    createDir(directory);

    strftime(timebuf, 32, "%H-%M-%S", timeInfo);
    snprintf(filename, 80, "%s/%s_Lvl%d.txt", directory, timebuf, prevState.level);

    FILE* file = fopen(filename, "w");

    if (file != NULL)
    {
        printf("Writing data to %s.\n", filename);
        fprintf(file, "%s\n", modeName);

        for (size_t i = 0; i < stateListSize; ++i)
        {
            struct tgm_state* current = &stateList[i];
            TgmToFumenState(current);

            fprintf(file, "%s,%d,%u,%d,%d,%d,%d,%d,%d\n",
                    GRADE_DISPLAY[(int)current->grade],
                    current->level,
                    current->timer,
                    current->tetromino,
                    current->xcoord,
                    current->ycoord,
                    current->rotation,
                    0,
                    current->inCreditRoll
                );
        }
        fclose(file);
    }
    else
    {
        fprintf(stderr, "Cannot write log to %s.\n", filename);
    }

    stateListSize = 0;
    gameModeAtStart = 0;
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
    // We want to detect /changes/ in game state.
    prevState = curState;
    readState(space, &curState);

    // Log placements
    if (fumen)
    {
        if (!inPlayingState(prevState.state) && inPlayingState(curState.state))
        {
            stateListSize = 0;
        }

        // TGMJ clears this game mode flag a few frames before the game starts.
        // Let's catch that final state change.
        if (prevState.gameMode != curState.gameMode && curState.gameMode == 0)
        {
            gameModeAtStart = prevState.gameMode;
        }

        // Piece is locked in
        if (inPlayingState(curState.state) && prevState.state == TGMJ_ACTIVE && curState.state == TGMJ_LOCKING)
        {
            pushStateToList(stateList, &stateListSize, &curState);
        }

        // Game is over
        if (inPlayingState(prevState.state) && !inPlayingState(curState.state))
        {
            writePlacementLog();
        }
    }

    if (tracker)
    {
        // Write current state to memory map
        struct tgm_state* mmapPtr = (struct tgm_state*)tgm_mm_getMapPointer();

	if (mmapPtr)
            *mmapPtr = curState;
    }
}
