#ifndef SYNTI2_MIDI_TRANSLATOR_H
#define SYNTI2_MIDI_TRANSLATOR_H

/* Some defines to make midi handling more clear in the code part.
 * These come directly from the MIDI standard: 
 */
#define MIDI_STATUS_NOTE_OFF 0x8
#define MIDI_STATUS_NOTE_ON 0x9
#define MIDI_STATUS_KEY_PRESSURE 0xa
#define MIDI_STATUS_CONTROL 0xb
#define MIDI_STATUS_PROGRAM 0xc
#define MIDI_STATUS_CHANNEL_PRESSURE 0xd
#define MIDI_STATUS_PITCH_WHEEL 0xe
#define MIDI_STATUS_SYSTEM 0xf

#include "synti2.h"

int
synti2_midi_to_misss(byte_t *midi_in, 
                     byte_t *misss_out, 
                     int input_size);

#endif
