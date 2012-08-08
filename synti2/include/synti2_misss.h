/** @file synti2_misss.h
 *
 * MIDI-like Interface for the Synti2 Software Synthesizer.
 *
 * MISSS_x are part of *internal* specification.
 * SYNTI2_SYSEX_x are part of *external* specification
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
 * par[0], for example)
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
 *
 * These are used within the SysEx wrappers, which is totally OK
 * (FIXME: is it really?)
 *
 * FIXME: these are not actually misss-specific, but also in the SysEx
 * that is visible to the world. So make these SYNTI2_SYSEX_ maybe?
 */
#define MISSS_OP_FILL_PATCHES 0
#define MISSS_OP_SET_3BIT 1
#define MISSS_OP_SET_7BIT 2
#define MISSS_OP_SET_F 3

#endif
