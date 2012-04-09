/** @file synti2_misss.h
 *
 * MIDI-like Interface for the Synti2 Software Synthesizer
 *
 */
#ifndef MISSS_INCLUDED
#define MISSS_INCLUDED

/*
 * Misss data format ideas:
 *
 * <songheader> <layer>+
 * <layer> = <layerheader> <layerdata>
 * <layerheader> = <length> <tickmultiplier> <type> <part#> <parameters>
 * <layerdata> = <event>+
 * <event> = <delta><byte>+
 *
 * ... more or less so... and... 
 *
 * Types of layers:
 *
 *  [- 0x8 note off
 *        <delta>  -- maybe unnecessary if using vel=0 for note off!]
 *  - 0x9 note on with variable velocity
 *        <delta><note#><velocity>
 *  - 0x1 note on with constant velocity (given as a parameter)
 *        (note off encoded as on with constant velocity==0)
 *        <delta><note#>
 *  - 0x2 note on with constant pitch (given as a parameter)
 *        <delta><velocity>
 *  - 0x3 note on with constant pitch and velocity (given as parameters)
 *        <delta>
 *
 *  OR... TODO: think if note on and note velocity could be separated?
 *              suppose they could.. could have accented beats and
 *              velocity ramps with little-ish overhead?
 *
 *  - 0xB controller instantaneous
 *        <delta><value>
 *  - 0x4 controller ramp from-to/during
 *        <delta><value1><delta2><value2>
 *  [- 0x5 pitch bend ramp from-to/during (or re-use controller ramp?
 *        DEFINITELY! because our values can be var-length which is 
 *        very natural for pitch bend MSB when needed..)
 *        <delta><bend1><delta2><bend2>]
 *  - 0xf sysex
 *        <delta><sysex>
 *
 *  - 0x6 0x7 0xA 0xC 0xD 0xE reserved. 0x5 probably too.
 *    Maybe could use the 4th bit of type nibble for something else?
 *
 *  Type and part fit in one byte.
 */


/* Hmm.. so far my file format contains only two kinds of layers (one
 * of which is implemented...) Layer storage is where we are space
 * limited the most, because the song data will be in layers. So maybe
 * these should be combined with other header information?
 */
#define MISSS_LAYER_NOTES 0x00
#define MISSS_LAYER_CONTROLLER_RAMPS 0x01

/* Only two bits needed for real-time synth control: */
#define MISSS_MSG_NOTE 0x00
#define MISSS_MSG_SETF 0x01
#define MISSS_MSG_DATA 0x02

/* FIXME: These are used in synti2 now, but not in sound editor!
  But... these are not actually misss-specific, but also in the
  SysEx that is visible to the world. So make these SYNTI2_MIDI_ 
 */
#define MISSS_OP_FILL_PATCHES 0
#define MISSS_OP_SET_3BIT 1
#define MISSS_OP_SET_7BIT 2
#define MISSS_OP_SET_F 3

#endif
