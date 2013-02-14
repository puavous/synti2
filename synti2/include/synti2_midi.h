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

#define MIDI_STATUS_SYSTEM_META 0xf

/* My own SysEx opcodes. Could name these SysEx or Syx, but I think
 * SEx sounds sexier.
 * 
 * Yep, and these need to be in yet another separate header..
 */
#define SYNTI2_SEX_PART_RESET     0
#define SYNTI2_SEX_PART_MODE      1
#define SYNTI2_SEX_PART_CHANLIST  2
#define SYNTI2_SEX_PART_CHANMAP   3
#define SYNTI2_SEX_PART_CONTROL   4
#define SYNTI2_SEX_PART_PITCHDEST 5
#define SYNTI2_SEX_PART_RECVNOFF  6
#define SYNTI2_SEX_PART_VELOCITY  7
#define SYNTI2_SEX_PART_INSTARAMP 8


#define SYNTI2_PART_MODE_UNISON      0
#define SYNTI2_PART_MODE_POLY_ROTATE 1
#define SYNTI2_PART_MODE_POLY_STACK  2
#define SYNTI2_PART_MODE_MAPPED      3
#define SYNTI2_PART_MODE_MUTE        4


#include "synti2.h"
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

int
synti2_midi_to_misss(byte_t *midi_in, 
                     byte_t *misss_out, 
                     int input_size);

#endif
