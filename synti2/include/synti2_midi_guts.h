#ifndef SYNTI2_MIDI_INTERNAL_STRUCTURES_H
#define SYNTI2_MIDI_INTERNAL_STRUCTURES_H

/* The array sizes come from here: */
#include "synti2_guts.h"

/** This structure is used for controlling the midi translator
 *  module. The sound engine is totally unaware of this.
 */
typedef struct {
  int use_sustain_pedal;      /* TODO: not yet implemented; maybe never?. */
  int receive_note_off;       /* 0=don't use note off; other=receive n-off. */
  int use_const_velocity;     /* 0=free; 1-127 force constant velocity. */
  int channels[NPARTS];       /* unison duplicates / polyphony (1..#voices)*/
  int note_channel_map[128];  /* For drum sounds and/or keyboard splits. */
  int bend_destination;       /* Controller to use for pitch bend. 0=none? */
  int mod_src[NCONTROLLERS];  /* 128 controllers (MIDI) to internal controls. */
  float mod_min[NCONTROLLERS]; /* destination-specific min value. */
  float mod_max[NCONTROLLERS]; /* destination-specific max value. */
  float instant_ramp_length;   /* length of "intantaneous" transition */
/*int cc_destination[128];*/  /* 128 controllers (MIDI) to internal controls. */
} synti2_midi_channel_map;

typedef struct {
  synti2_midi_channel_map chn[NPARTS];
} synti2_midi_map;

#endif
