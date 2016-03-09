#ifndef TGMTRACKER_H
#define TGMTRACKER_H

#include <stdbool.h>

typedef struct running_machine running_machine;
typedef struct _address_space address_space;

struct tgmtracker_t
{
    void (*setAddressSpace)(running_machine* machine);
    void (*initialize)();
    void (*cleanup)();
    void (*run)(bool fumen, bool tracker);
};

const address_space* tt_setAddressSpace(running_machine* machine);

extern struct tgmtracker_t tgm2p_tracker;
extern struct tgmtracker_t tgmj_tracker;

#endif /* TGMTRACKER_H */
