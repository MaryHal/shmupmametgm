#ifndef TGM2P_HANDLER_H
#define TGM2P_HANDLER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct running_machine running_machine;

void tgm2p_create_mmap();
void tgm2p_destroy_mmap();

void tgm2p_setAddressSpace(running_machine* machine);
void tgm2p_run(bool fumen, bool tracker);

#endif /* TGM2P_HANDLER_H */
