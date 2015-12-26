#ifndef FUMEN_H
#define FUMEN_H

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
};

typedef struct running_machine running_machine;

void tetlog_setAddressSpace(running_machine* machine);
void tetlog_create_mmap();
void tetlog_destroy_mmap();
void tetlog_run(bool fumen, bool tracker);

#endif /* FUMEN_H */
