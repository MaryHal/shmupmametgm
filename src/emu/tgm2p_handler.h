#ifndef TGM2P_HANDLER_H
#define TGM2P_HANDLER_H

#include <stdbool.h>
#include <stdint.h>

struct tap_state
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

typedef struct running_machine running_machine;

void tgm2p_create_mmap();
void tgm2p_destroy_mmap();

void tgm2p_setAddressSpace(running_machine* machine);
void tgm2p_run(bool fumen, bool tracker);

#endif /* TGM2P_HANDLER_H */
