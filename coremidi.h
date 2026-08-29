#ifndef _COREMIDI_H_
#define _COREMIDI_H_

#include <stdint.h>
#include <stddef.h>

int coremidi_setup(const char *name);
void coremidi_close(void);
void coremidi_write(uint8_t *data, size_t datalen);

#endif