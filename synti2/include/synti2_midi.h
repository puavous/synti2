#ifndef SYNTI2_MIDI_TRANSLATOR_H
#define SYNTI2_MIDI_TRANSLATOR_H

#include "synti2.h"
#include "synti2_archdep.h"

int
synti2_midi_to_misss(synti2_synth *s,
                     const byte_t *midi_in, 
                     byte_t *misss_out, 
                     int *msgsize,
                     int input_size);

#endif
