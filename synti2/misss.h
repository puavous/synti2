/** @file misss.h
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


/* These are pretty much fixed, as the implementation depends on bit
 * patterns!
 */
/* Layer identifier nibbles 0-7 */
#define MISSS_LAYER_NOTES 0x00
#define MISSS_LAYER_NOTES_CPITCH 0x02
#define MISSS_LAYER_NOTES_CVEL 0x01
#define MISSS_LAYER_NOTES_CVEL_CPITCH 0x03
/* TODO: Do I need two different controller commands, or could I use
 * just one, somehow parameterized to be either reset or ramp each? 
 * Hmm... Hey: reset is a ramp with zero time, isn't it?! 
 */
#define MISSS_LAYER_CONTROLLER_RESETS 0x04
#define MISSS_LAYER_CONTROLLER_RAMPS 0x05
#define MISSS_LAYER_SYSEX_OR_ARBITRARY 0x06
#define MISSS_LAYER_NOTHING_AS_OF_YET 0x07

#endif
