#ifndef TGM_COMMON_H
#define TGM_COMMON_H

#include <stdint.h>
#include <stddef.h>

struct tgm_state
{
    int16_t state;
    int16_t grade;
    int16_t gradePoints;

    int16_t level;
    int16_t timer;

    int16_t tetromino;
    int16_t xcoord;
    int16_t ycoord;
    int16_t rotation;
    int16_t mrollFlags;
    int16_t inCreditRoll;

    int16_t gameMode;
};

void TgmToFumenState(struct tgm_state* tstate);

// Helper Functions
int createDir(const char* path);

// TODO: Find the attract mode data!
bool testDemoState(struct tgm_state* stateList, size_t listLength, struct tgm_state* demo, size_t demoLength);

#endif /* TGM_COMMON_H */
