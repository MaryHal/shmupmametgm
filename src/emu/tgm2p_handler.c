#include "tgm2p_handler.h"

#include "tgmtracker.h"
#include "tgm_memorymap.h"

#include "tgm_common.h"

#include <stdlib.h>
#include <time.h>

#include "emu.h"
#include "debug/express.h"

void tgm2p_create_mmap()
{
    tgm_mm_create(sizeof(struct tgm_state));
}

void tgm2p_destroy_mmap()
{
    tgm_mm_destroy();
}

static const int GRADE_COUNT = 32;
static const char* GRADE_DISPLAY[GRADE_COUNT] =
{
    "9", "8", "7", "6", "5", "4-", "4+", "3-", "3+", "2-", "2", "2+", "1-",
    "1", "1+", "S1-", "S1", "S1+", "S2", "S3", "S4-", "S4", "S4+", "S5-",
    "S5+", "S6-", "S6+", "S7-", "S7+", "S8-", "S8+", "S9"
};

enum tap_internal_state
{
    TAP_NONE         = 0,
    TAP_START        = 1,
    TAP_ACTIVE       = 2,
    TAP_LOCKING      = 3,  // Cannot be influenced anymore
    TAP_LINECLEAR    = 4,  // Tetromino is being locked to the playfield.
    TAP_ENTRY        = 5,
    TAP_GAMEOVER     = 7,  // "Game Over" is being shown on screen.
    TAP_IDLE         = 10, // No game has started, just waiting...
    TAP_FADING       = 11, // Blocks fading away when topping out (losing).
    TAP_COMPLETION   = 13, // Blocks fading when completing the game
    TAP_STARTUP      = 71,
};

enum tap_mroll_flags
{
    M_FAIL_BOTH_1   = 0,   // 00000000 Failing (both) at 100
    M_FAIL_TETRIS_1 = 17,  // 00010001 Failing (tetris) at 100
    M_FAIL_TIME_1   = 32,  // 00100000 Failing (time) at 100

    M_FAIL_BOTH_2   = 1,   // 00000001 Failing (both) between 100 - 500
    M_FAIL_TETRIS_2 = 17,  // 00010001 Failing (tetris) between 100 - 500
    M_FAIL_TIME_2   = 33,  // 00010001 Failing (time) between 100 - 500

    M_FAIL_3        = 19,  // 00010011 Failing at 500
    M_FAIL_END      = 31,  // 00011111 Failing at 999

    M_NEUTRAL       = 48,  // 00110000 Default State
    M_PASS_1        = 49,  // 00110001 Passing at 100
    M_PASS_2        = 51,  // 00110011 Passing at 500
    M_SUCCESS       = 127, // 01111111 Passing at 999
};

// Considering how you can fail on time in the first section with a value of 32,
// this doesn't seem to work 100%
#define MROLL_PASS_MASK (1 << 5)

// Non-exhaustive list of game modes
enum tap_game_mode
{
    TAP_MODE_NULL           = 0,
    TAP_MODE_NORMAL         = 1,
    TAP_MODE_MASTER         = 2,
    TAP_MODE_DOUBLES        = 4,
    TAP_MODE_NORMAL_VERSUS  = 9,
    TAP_MODE_MASTER_VERSUS  = 10,
    TAP_MODE_MASTER_CREDITS = 18,
    TAP_MODE_NORMAL_20G     = 33,
    TAP_MODE_MASTER_20G     = 34,
    TAP_MODE_DOUBLES_20G    = 36,
    TAP_MODE_TGMPLUS        = 128,
    TAP_MODE_TGMPLUS_VERSUS = 136,
    TAP_MODE_TGMPLUS_20G    = 160,
    TAP_MODE_MASTER_ITEM    = 514,
    TAP_MODE_TGMPLUS_ITEM   = 640,
    TAP_MODE_DEATH          = 4096,
    TAP_MODE_DEATH_VERSUS   = 4104,
    TAP_MODE_DEATH_20G      = 4128
};

#define MODE_VERSUS_MASK  (1 << 3)
#define MODE_CREDITS_MASK (1 << 4)
#define MODE_20G_MASK     (1 << 5)
#define MODE_BIG_MASK     (1 << 6)
#define MODE_ITEM_MASK    (1 << 9)
#define MODE_TLS_MASK     (1 << 10)

static bool isVersusMode(int gameMode)
{
    return gameMode & MODE_VERSUS_MASK;
}

static bool is20GMode(int gameMode)
{
    return gameMode & MODE_20G_MASK;
}

static bool isBigMode(int gameMode)
{
    return gameMode & MODE_BIG_MASK;
}

static bool isItemMode(int gameMode)
{
    return gameMode & MODE_ITEM_MASK;
}

static bool isTLSMode(int gameMode)
{
    return gameMode & MODE_TLS_MASK;
}

static int getBaseMode(int gameMode)
{
    int megaModeMask =
        MODE_VERSUS_MASK  |
        MODE_CREDITS_MASK |
        MODE_20G_MASK     |
        MODE_BIG_MASK     |
        MODE_ITEM_MASK    |
        MODE_TLS_MASK;

    return gameMode & ~megaModeMask;
}

static void getModeName(char* buffer, size_t bufferLength, int gameMode)
{
    const uint8_t BUF_SIZE = 16;

    char modifierMode[BUF_SIZE] = "";
    if (isVersusMode(gameMode))
    {
        strncpy(modifierMode, "Versus_", BUF_SIZE);
    }
    else if (is20GMode(gameMode))
    {
        strncpy(modifierMode, "20G_", BUF_SIZE);
    }
    else if (isBigMode(gameMode))
    {
        strncpy(modifierMode, "Big_", BUF_SIZE);
    }
    else if (isItemMode(gameMode))
    {
        strncpy(modifierMode, "Item_", BUF_SIZE);
    }
    else if (isTLSMode(gameMode))
    {
        strncpy(modifierMode, "TLS_", BUF_SIZE);
    }

    char baseMode[BUF_SIZE] = "";
    switch (getBaseMode(gameMode))
    {
    case TAP_MODE_NULL:
        strncpy(baseMode, "NULL", BUF_SIZE);
        break;
    case TAP_MODE_NORMAL:
        strncpy(baseMode, "Normal", BUF_SIZE);
        break;
    case TAP_MODE_MASTER:
        strncpy(baseMode, "Master", BUF_SIZE);
        break;
    case TAP_MODE_DOUBLES:
        strncpy(baseMode, "Doubles", BUF_SIZE);
        break;
    case TAP_MODE_TGMPLUS:
        strncpy(baseMode, "TGM+", BUF_SIZE);
        break;
    case TAP_MODE_DEATH:
        strncpy(baseMode, "Death", BUF_SIZE);
        break;
    default:
        strncpy(baseMode, "???", BUF_SIZE);
    }

    snprintf(buffer, bufferLength, "%s%s", modifierMode, baseMode);
}

static bool testMasterConditions(char flags)
{
    return
        flags == M_NEUTRAL ||
        flags == M_PASS_1  ||
        flags == M_PASS_2  ||
        flags == M_SUCCESS;
}

static bool inPlayingState(char state)
{
    return state != TAP_NONE && state != TAP_IDLE && state != TAP_STARTUP;
}

static const offs_t STATE_ADDR       = 0x06064BF5;  // p1 State
static const offs_t LEVEL_ADDR       = 0x06064BBA;  // p1 Level
static const offs_t TIMER_ADDR       = 0x06064BEA;  // p1 Timer

static const offs_t GRADE_ADDR       = 0x06079378;  // Master-mode internal grade
static const offs_t GRADEPOINTS_ADDR = 0x06079379;  // Master-mode internal grade points
static const offs_t MROLLFLAGS_ADDR  = 0x06064BD0;  // M-Roll flags
static const offs_t INROLL_ADDR      = 0x06066845;  // p1 in-credit-roll
static const offs_t SECTION_ADDR     = 0x06064C25;  // p1 section index

static const offs_t TETRO_ADDR       = 0x06064BF6;  // Current block
static const offs_t NEXT_ADDR        = 0x06064BF8;  // Next block
static const offs_t CURRX_ADDR       = 0x06064BFC;  // Current block X position
static const offs_t CURRY_ADDR       = 0x06064C00;  // Current block Y position
static const offs_t ROTATION_ADDR    = 0x06064BFA;  // Current block rotation state

static const offs_t GAMEMODE_ADDR    = 0x06064BA4;  // Current game mode

static void readState(const address_space* space, struct tgm_state* state)
{
    state->state        = memory_read_byte(space, STATE_ADDR);
    state->grade        = memory_read_byte(space, GRADE_ADDR);
    state->gradePoints  = memory_read_byte(space, GRADEPOINTS_ADDR);

    state->level        = memory_read_word(space, LEVEL_ADDR);
    state->timer        = memory_read_word(space, TIMER_ADDR);

    state->tetromino    = memory_read_word(space, TETRO_ADDR);
    state->xcoord       = memory_read_word(space, CURRX_ADDR);
    state->ycoord       = memory_read_word(space, CURRY_ADDR);
    state->rotation     = memory_read_byte(space, ROTATION_ADDR);

    state->mrollFlags   = memory_read_byte(space, MROLLFLAGS_ADDR);
    state->inCreditRoll = memory_read_byte(space, INROLL_ADDR);

    state->gameMode = memory_read_word(space, GAMEMODE_ADDR);
}

static void pushStateToList(struct tgm_state* list, size_t* listSize, struct tgm_state* state)
{
    /* state->tetromino = TgmToFumenMapping[state->tetromino]; */
    /* TgmToFumenState(state); */

    list[*listSize] = *state;
    (*listSize)++;
}

// First Demo: Two simultaneous single player games.
static const size_t demo01_length = 16;
static struct tgm_state demo01[] =
{
    { 2, 0, 0, 0, 26, 5, 1, 3, 2, 48, 0, 2 },
    { 2, 0, 0, 1, 75, 8, 4, 3, 2, 48, 0, 2 },
    { 2, 0, 0, 2, 137, 7, 6, 3, 0, 48, 0, 2 },
    { 2, 0, 0, 3, 226, 6, 2, 4, 2, 48, 0, 2 },
    { 2, 0, 0, 4, 291, 4, 2, 5, 0, 48, 0, 2 },
    { 2, 0, 0, 5, 384, 5, 7, 4, 0, 48, 0, 2 },
    { 2, 0, 0, 6, 438, 8, 5, 4, 3, 48, 0, 2 },
    { 2, 0, 0, 7, 491, 3, 0, 5, 1, 48, 0, 2 },
    { 2, 0, 0, 8, 542, 2, 8, 4, 3, 48, 0, 2 },
    { 2, 0, 20, 11, 635, 4, 4, 4, 1, 48, 0, 2 },
    { 2, 0, 20, 12, 692, 7, 6, 4, 0, 48, 0, 2 },
    { 2, 0, 20, 13, 759, 3, 0, 5, 1, 48, 0, 2 },
    { 2, 0, 20, 14, 827, 6, 6, 5, 0, 48, 0, 2 },
    { 2, 0, 19, 15, 883, 2, 7, 4, 1, 48, 0, 2 },
    { 2, 0, 39, 18, 969, 8, 4, 4, 1, 48, 0, 2 },
    { 2, 0, 39, 19, 1035, 5, 2, 3, 1, 48, 0, 2 }
};

// Second Demo: Vs Mode.
static const size_t demo02_length = 14;
static struct tgm_state demo02[] =
{
    { 2, 0, 0, 0, 9554, 6, 1, 3, 2, 48, 0, 10 },
    { 2, 0, 0, 1, 9493, 5, 4, 3, 2, 48, 0, 10 },
    { 2, 0, 0, 2, 9444, 2, -1, 5, 3, 48, 0, 10 },
    { 2, 0, 0, 3, 9396, 4, 4, 4, 1, 48, 0, 10 },
    { 2, 0, 0, 4, 9326, 3, 1, 4, 1, 48, 0, 10 },
    { 2, 0, 0, 5, 9265, 8, 1, 6, 1, 48, 0, 10 },
    { 2, 0, 0, 6, 9184, 7, 6, 3, 0, 48, 0, 10 },
    { 2, 0, 0, 7, 9125, 6, 6, 4, 0, 48, 0, 10 },
    { 2, 0, 0, 8, 9075, 5, 8, 3, 1, 48, 0, 10 },
    { 2, 0, 10, 10, 8976, 2, 5, 4, 0, 48, 0, 10 },
    { 2, 0, 10, 11, 8891, 3, 3, 9, 0, 48, 0, 10 },
    { 2, 0, 10, 12, 8805, 7, 5, 9, 0, 48, 0, 10 },
    { 2, 0, 9, 13, 8754, 4, 9, 7, 3, 48, 0, 10 },
    { 2, 0, 29, 16, 8667, 8, 0, 8, 1, 48, 0, 10 }
};

// Third Demo: Doubles Mode.
static const size_t demo03_length = 14;
static struct tgm_state demo03[] =
{
    { 2, 0, 0, 0, 32, 5, 1, 3, 2, 48, 0, 4 },
    { 2, 0, 0, 1, 128, 8, 4, 3, 2, 48, 0, 4 },
    { 2, 0, 0, 2, 214, 6, 2, 4, 2, 48, 0, 4 },
    { 2, 0, 0, 3, 270, 7, 0, 5, 0, 48, 0, 4 },
    { 2, 0, 0, 4, 346, 2, 5, 4, 1, 48, 0, 4 },
    { 2, 0, 0, 5, 442, 3, 2, 5, 1, 48, 0, 4 },
    { 2, 0, 0, 6, 512, 5, 1, 6, 2, 48, 0, 4 },
    { 2, 0, 0, 7, 589, 6, 2, 7, 2, 48, 0, 4 },
    { 2, 0, 0, 8, 656, 8, 5, 3, 3, 48, 0, 4 },
    { 2, 0, 20, 11, 754, 4, 2, 6, 0, 48, 0, 4 },
    { 2, 0, 20, 12, 811, 3, 0, 5, 1, 48, 0, 4 },
    { 2, 0, 20, 13, 903, 2, 4, 6, 1, 48, 0, 4 },
    { 2, 0, 19, 14, 989, 7, 2, 7, 0, 48, 0, 4 },
    { 2, 0, 19, 15, 1050, 8, 0, 7, 1, 48, 0, 4 }
};

static bool testDemoState(struct tgm_state* stateList, size_t listSize, struct tgm_state* demo, size_t demoSize)
{
    if (listSize > demoSize)
    {
        return false;
    }

    int misses = 0;

    size_t sCount = 0;
    size_t dCount = 0;
    for (; sCount < listSize && dCount < demoSize; ++dCount)
    {
        if (stateList[sCount].gameMode  != demo[dCount].gameMode  ||
            stateList[sCount].tetromino != demo[dCount].tetromino ||
            stateList[sCount].xcoord    != demo[dCount].xcoord    ||
            stateList[sCount].ycoord    != demo[dCount].ycoord)
        {
            misses++;

            if (misses >= 6)
            {
                return false;
            }
        }
        else
        {
            sCount++;
        }
    }
    return true;
}

static bool isDemoState(struct tgm_state* stateList, size_t listSize)
{
    return
        testDemoState(stateList, listSize, demo01, demo01_length) ||
        testDemoState(stateList, listSize, demo02, demo02_length) ||
        testDemoState(stateList, listSize, demo03, demo03_length);
}

static struct tgm_state curState = {0}, prevState = {0};

static const size_t MAX_TGM_STATES = 1300; // What a nice number
static struct tgm_state stateList[MAX_TGM_STATES];
static size_t stateListSize = 0;

static int gameModeAtStart = 0;

static const address_space* space = NULL;

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
    // since, upon death, TAP clears some data.
    pushStateToList(stateList, &stateListSize, &prevState);

    // Create fumen directory if it doesn't exist.
    createDir("fumen/");
    createDir("fumen/tgm2p");

    char directory[32];
    char timebuf[32];
    char filename[80];

    // Create a directory for the day if it doesn't already exist.
    time_t rawTime;
    time(&rawTime);
    const struct tm* timeInfo = localtime(&rawTime);
    strftime(directory, 32, "fumen/tgm2p/%Y-%m-%d", timeInfo);

    char modeName[32];
    getModeName(modeName, 32, gameModeAtStart);

    createDir(directory);

    strftime(timebuf, 32, "%H-%M-%S", timeInfo);
    snprintf(filename, 80, "%s/%s_%s_Lvl%d.txt", directory, timebuf, modeName, prevState.level);

    FILE* file = fopen(filename, "w");

    if (file != NULL)
    {
        printf("Writing data to %s.\n", filename);

        fprintf(file, "%s\n", modeName);

        for (size_t i = 0; i < stateListSize; ++i)
        {
            stateList[i].tetromino = TgmToFumenMapping[stateList[i].tetromino];
            TgmToFumenState(&stateList[i]);

            struct tgm_state* current = &stateList[i];
            fprintf(file, "%s,%d,%d,%d,%d,%d,%d,%d,%d\n",
                    GRADE_DISPLAY[(int)current->grade],
                    current->level,
                    current->timer,
                    current->tetromino,
                    current->xcoord,
                    current->ycoord,
                    current->rotation,
                    testMasterConditions(current->mrollFlags),
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
}

void tgm2p_setAddressSpace(running_machine* machine)
{
    space = tt_setAddressSpace(machine);
}

void tgm2p_run(bool fumen, bool tracker)
{
    // We want to detect /changes/ in game state.
    prevState = curState;
    readState(space, &curState);

    // Log placements
    if (fumen)
    {
        // Game has begun, save the game mode since tgm2p removes mode
        // modifiers when the game ends.
        if (!inPlayingState(prevState.state) && inPlayingState(curState.state))
            gameModeAtStart = curState.gameMode;

        // Piece is locked in
        if (inPlayingState(curState.state) && prevState.state == TAP_ACTIVE && curState.state == TAP_LOCKING)
        {
            pushStateToList(stateList, &stateListSize, &curState);
        }

        // Game is over
        if (inPlayingState(prevState.state) && !inPlayingState(curState.state))
        {
            writePlacementLog();
        }
    }

    // Write current tap state to memory mapped file
    if (tracker)
    {
        struct tgm_state* mmapPtr = (struct tgm_state*)tgm_mm_getMapPointer();
        if (mmapPtr)
            *mmapPtr = curState;
    }
}
