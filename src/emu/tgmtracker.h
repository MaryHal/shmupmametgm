#ifndef TAPTRACKER_H
#define TAPTRACKER_H

#include <stdbool.h>

typedef struct running_machine running_machine;

struct tgmtracker_t
{
    void (*setAddressSpace)(running_machine* machine);
    void (*initialize)();
    void (*cleanup)();
    void (*run)(bool fumen, bool tracker);
};

extern struct tgmtracker_t tgm2p_tracker;
extern struct tgmtracker_t tgmj_tracker;

#endif /* TAPTRACKER_H */
