#ifndef SYNTI2_MIDI_TRANSLATOR_H
#define SYNTI2_MIDI_TRANSLATOR_H

#include "synti2.h"

/* Worst-case data sizes for translating one incoming midi message
 * into a series of misss messages. Bytes buffer is bigger than
 * usually required, to make space for longer SysExes (at least 1024
 * bytes, which is basically very long for a MIDI SysEx anyway).
 */
#define MISSS_MAX_MESSAGES  NUM_CHANNELS
#define MISSS_MAX_BYTES     (1024*(MISSS_MAX_MESSAGES)*(3+2*sizeof(float)))

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
