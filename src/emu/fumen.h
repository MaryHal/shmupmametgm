#ifndef FUMEN_H
#define FUMEN_H

#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdbool.h>
#include <stdint.h>

#include "debug/debugcpu.h"

enum
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
    TAP_STARTUP      = 71
};

enum
{
    M_FAIL_1   = 17,
    M_FAIL_2   = 19,
    M_FAIL_END = 31,

    M_NEUTRAL  = 48,
    M_PASS_1   = 49,
    M_PASS_2   = 51,
    M_SUCCESS  = 127,
};


const int STATE_ADDR       = 0x06064BF5;  // p1 State
const int LEVEL_ADDR       = 0x06064BBA;  // p1 Level
const int TIMER_ADDR       = 0x06064BEA;  // p1 Timer
const int GRADE_ADDR       = 0x06079378;  // Master-mode internal grade
const int GRADEPOINTS_ADDR = 0x06079379;  // Master-mode internal grade points
const int MROLLFLAGS_ADDR  = 0x06064BD0;  // M-Roll flags
const int INROLL_ADDR      = 0x06066845;  // p1 in-credit-roll
const int SECTION_ADDR     = 0x06064C25;  // p1 section index

const int TETRO_ADDR       = 0x06064BF6;  // Current block
const int NEXT_ADDR        = 0x06064BF8;  // Next block
const int CURRX_ADDR       = 0x06064BFC;  // Current block X position
const int CURRY_ADDR       = 0x06064C00;  // Current block Y position
const int ROTATION_ADDR    = 0x06064BFA;  // Current block rotation state

struct tap_state
{
        char state;
        int level;
        int timer;

        int tetromino;
        int xcoord;
        int ycoord;
        char rotation;

        char mrollFlags;
};

// TGM2+ indexes its pieces slightly differently to fumen, so when encoding a
// diagram we must convert the indices:
// 2 3 4 5 6 7 8 (TAP)
// I Z S J L O T
// 1 4 7 6 2 3 5 (Fumen)
char TapToFumenMapping[9] = {0, 0, 1, 4, 7, 6, 2, 3, 5};

// Coordinates from TAP do not align perfectly with fumen's coordinates
// (depending on tetromino and rotation state).
void fixTapCoordinates(struct tap_state* tstate)
{
    if (tstate->tetromino == 1)
    {
        // Fix underflow when I tetromino is in column 1.
        if (tstate->xcoord > 10)
        {
            tstate->xcoord = -1;
        }

        if (tstate->rotation == 1 || tstate->rotation == 3)
        {
            tstate->xcoord += 1;
        }
    }
    else if (tstate->tetromino == 6)
    {
        if (tstate->rotation == 2)
        {
            tstate->ycoord -= 1;
        }
    }
    else if (tstate->tetromino == 2)
    {
        if (tstate->rotation == 2)
        {
            tstate->ycoord -= 1;
        }
    }
    else if (tstate->tetromino == 5)
    {
        if (tstate->rotation == 2)
        {
            tstate->ycoord -= 1;
        }
    }
}

char testMasterConditions(char d)
{
    return d == M_NEUTRAL || d == M_PASS_1 || d == M_PASS_2 || d == M_SUCCESS;
}

char inPlayingState(char state)
{
    return state != TAP_NONE && state != TAP_IDLE && state != TAP_STARTUP;
}

struct tap_state curState = {0}, prevState = {0};

const int MAX_TAP_STATES = 1300; // What a nice number
struct tap_state stateList[MAX_TAP_STATES];
unsigned int stateListSize = 0;

running_device* maincpu_device = NULL;

void readState(const address_space* space, struct tap_state* state)
{
    state->state      = debug_read_byte(space, memory_address_to_byte(space, STATE_ADDR), TRUE);
    state->level      = debug_read_word(space, memory_address_to_byte(space, LEVEL_ADDR), TRUE);
    state->timer      = debug_read_word(space, memory_address_to_byte(space, TIMER_ADDR), TRUE);

    state->tetromino  = debug_read_word(space, memory_address_to_byte(space, TETRO_ADDR), TRUE);
    state->xcoord     = debug_read_word(space, memory_address_to_byte(space, CURRX_ADDR), TRUE);
    state->ycoord     = debug_read_word(space, memory_address_to_byte(space, CURRY_ADDR), TRUE);
    state->rotation   = debug_read_byte(space, memory_address_to_byte(space, ROTATION_ADDR), TRUE);

    state->mrollFlags = debug_read_byte(space, memory_address_to_byte(space, MROLLFLAGS_ADDR), TRUE);
}

void pushState(struct tap_state* list, unsigned int* listSize, struct tap_state* state)
{
    state->tetromino = TapToFumenMapping[state->tetromino];

    fixTapCoordinates(state);

    list[*listSize] = *state;
    (*listSize)++;
}

void findAndSetMainCpu(running_machine* machine)
{
    if (maincpu_device == NULL)
    {
        // Search for maincpu
        for (maincpu_device = machine->devicelist.first(); maincpu_device != NULL; maincpu_device = maincpu_device->next)
        {
            if (mame_stricmp(maincpu_device->tag(), "maincpu") == 0)
            {
                break;
            }
        }
    }
}

void runTetrominoLogger()
{
    if (!maincpu_device)
        return;

    const address_space* space = cpu_get_address_space(maincpu_device, ADDRESS_SPACE_PROGRAM + (0 - EXPSPACE_PROGRAM_LOGICAL));

    // We want to detect /changes/ in game state.
    prevState = curState;
    readState(space, &curState);

    // Piece is locked in
    if (inPlayingState(curState.state) && prevState.state == TAP_ACTIVE && curState.state == TAP_LOCKING)
    {
        pushState(stateList, &stateListSize, &curState);
    }

    // Game is over
    if (inPlayingState(prevState.state) && !inPlayingState(curState.state))
    {
        // Push the killing piece. We must use the previous state
        // since, upon death, TAP clears some data.
        pushState(stateList, &stateListSize, &prevState);

        struct stat st = {0};

        // Create autofumen directory if it doesn't exist.
        if (stat("fumen/", &st) == -1)
        {
            mkdir("fumen/", 0700);
        }

        char directory[80];
        char timebuf[80];
        char filename[80];

        // Create a directory for the day if it doesn't already exist.
        time_t rawTime;
        time(&rawTime);
        struct tm* timeInfo = localtime(&rawTime);
        strftime(directory, 80, "fumen/%F", timeInfo);
        if (stat(directory, &st) == -1)
        {
            mkdir(directory, 0700);
        }

        strftime(timebuf, 80, "%H:%M:%S", timeInfo);
        snprintf(filename, 80, "%s/%s-Lvl%d.txt", directory, timebuf, stateList[stateListSize - 1].level);

        printf("Wrote data to %s\n", filename);

        FILE* file = fopen(filename, "w");

        if (file != NULL)
        {
            for (unsigned int i = 0; i < stateListSize; ++i)
            {
                struct tap_state* current = &stateList[i];
                fprintf(file, "%d,%d,%d,%d,%d,%d,%d\n",
                        current->level,
                        current->timer,
                        current->tetromino,
                        current->xcoord,
                        current->ycoord,
                        current->rotation,
                        testMasterConditions(current->mrollFlags)
                    );
            }
        }
        fclose(file);
    }
}

#endif /* FUMEN_H */
