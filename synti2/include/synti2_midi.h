#ifndef SYNTI2_MIDI_TRANSLATOR_H
#define SYNTI2_MIDI_TRANSLATOR_H

#include "synti2.h"
#include "synti2_archdep.h"

typedef struct synti2_midi_map synti2_midi_map;
typedef struct synti2_midi_state synti2_midi_state;

int
synti2_midi_to_misss(synti2_midi_map *map,
                     synti2_midi_state *state,
                     const unsigned char *midi_in,
                     unsigned char *misss_out,
                     int *msgsize,
                     int input_size);

#endif
