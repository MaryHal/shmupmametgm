#ifndef FUMEN_H
#define FUMEN_H

#include <stdbool.h>

typedef struct running_machine running_machine;

void tetlog_setAddressSpace(running_machine* machine);
void tetlog_create_mmap();
void tetlog_destroy_mmap();
void tetlog_run(bool fumen, bool tracker);

#endif /* FUMEN_H */
