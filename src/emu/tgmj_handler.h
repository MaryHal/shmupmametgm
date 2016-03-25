#ifndef TGMJ_HANDLER_H
#define TGMJ_HANDLER_H

#include <stdbool.h>
#include <stdint.h>

struct tgmj_state
{
    int16_t level;
    int16_t timer;
};

typedef struct running_machine running_machine;

void tgmj_create_mmap();
void tgmj_destroy_mmap();

void tgmj_setAddressSpace(running_machine* machine);
void tgmj_run(bool fumen, bool tracker);

#endif /* TGMJ_HANDLER_H */
