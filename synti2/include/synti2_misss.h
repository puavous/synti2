/** @file synti2_misss.h
 *
 * MIDI-like Interface for the Synti2 Software Synthesizer.
 *
 * MISSS_x are part of *internal* specification. 
 *
 * MISSS_SYSEX_x are part of both internal and also *external*
 * specification (directly passed on to the engine in compose mode).
 * Hmm.. that means that they should be in their own header file..
 */
#ifndef MISSS_INCLUDED
#define MISSS_INCLUDED

/* Layer types (stored). Space-limited.
 *
 * Hmm.. so far my file format contains only two kinds of layers (one
 * of which is implemented...) Layer storage is where we are space
 * limited the most, because the song data will be in layers. So maybe
 * these should be combined with other header information?
 *
 * FIXME: If only one bit is used for layer identification in the end,
 * then maybe use the remaining 7 for some useful aspect (could hold
 * par[0], for example). But... do I want _LAYER_DATA !?
 */
#define MISSS_LAYER_NOTES 0x00
#define MISSS_LAYER_CONTROLLER_RAMPS 0x01

/* Message types (playable). Not space-limited (Generated at
 * run-time).
 *
 * FIXME: Are SETF or DATA really needed? For these we have real-time
 * operations (SET_x)
 */
#define MISSS_MSG_NOTE 0x00
#define MISSS_MSG_RAMP 0x01
#define MISSS_MSG_SETF 0x02
#define MISSS_MSG_DATA 0x03

/* Operations (instantaneous) at compose-time. Not space-limited.
 * These are in the SysEx specification, visible to the world.
 */
#define MISSS_SYSEX_SET_3BIT 0
#define MISSS_SYSEX_SET_7BIT 1
#define MISSS_SYSEX_SET_F    2

#define MISSS_SYSEX_MM_SUST      9
#define MISSS_SYSEX_MM_MODE      10
#define MISSS_SYSEX_MM_NOFF      11
#define MISSS_SYSEX_MM_CVEL      12
#define MISSS_SYSEX_MM_VOICES    13
#define MISSS_SYSEX_MM_MAPSINGLE 14
#define MISSS_SYSEX_MM_MAPALL    15
#define MISSS_SYSEX_MM_BEND      16
#define MISSS_SYSEX_MM_PRESSURE  17
#define MISSS_SYSEX_MM_MODDATA   18
#define MISSS_SYSEX_MM_RAMPLEN   22


/* Channel modes. */
#define MM_MODE_DUP     0
#define MM_MODE_POLYROT 1
#define MM_MODE_MAPPED  2
#define MM_MODE_MUTE    3

#endif
