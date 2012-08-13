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
#include "synti2_guts.h"

typedef struct {
  int slave_channels;         /* polyphony */
  int slave_channel_alg;      /* algorithm for poly allocate. 0=rotate; */
  int use_sustain_pedal;      /* TODO: not yet implemented; maybe never?. */
  int block_note_off;         /* 0=use note off; other=don't use n-off. */
  int constant_velocity;      /* 0=free; 1-127 force constant velocity. */
  int bend_destination;       /* Controller to use for pitch bend. */
  int cc_destination[128];    /* 128 controllers (MIDI) to internal controls. */
  float cc_min[NCONTROLLERS]; /* destination-specific min value. */
  float cc_max[NCONTROLLERS]; /* destination-specific max value. */
  float instant_ramp_length;  /* length of "intantaneous" transition */
} synti2_midi_channel_map;

typedef struct {
  synti2_midi_channel_map chn[NPARTS];
} synti2_midi_map;


int
synti2_midi_to_misss(byte_t *midi_in, 
                     byte_t *misss_out, 
                     int input_size);

#endif
