#include "tgm_common.h"

// TGM indexes its pieces slightly differently to fumen, so when encoding a
// diagram we must convert the indices:
// 2 3 4 5 6 7 8 (TAP)
// I Z S J L O T
// 1 4 7 6 2 3 5 (Fumen)
uint8_t TgmToFumenMapping[9] = { 0, 0, 1, 4, 7, 6, 2, 3, 5 };

// Coordinates from TAP do not align perfectly with fumen's coordinates
// (depending on tetromino and rotation state).
void TgmToFumenState(struct tgm_state* tstate)
{
    if (tstate->tetromino == 1)
    {
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
