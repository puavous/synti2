#ifndef SYNTI2_CAPACITIES_INCLUDED
#define SYNTI2_CAPACITIES_INCLUDED

/** @file synti2_cap_default.h 
 * 
 * This file defines and documents the "capacities", or call them
 * limits, of the Synti2 software synthesizer. The capacities are
 * compiled into the executable synthesizer, and the idea is that more
 * restricted custom values can be used in compiling the final 4k
 * target. This file sets "default" values that can be used for a
 * real-time synthesizer while composing music and desigining patches.
 *
 */

/* Multitimbrality. This equals polyphonic capacity, too. This
 * decision makes the code simpler and resultingly smaller for the 4k
 * intro use case, because no code needs to be written for dynamically
 * allocating a "voice". If there is need for more polyphony, it would
 * be easy enough to create several synth instances running in
 * parallel, each giving a maximum of 16 new "voices" to the whole.
 *
 * (And I would still like to call these "channels" rather than
 * "parts", but let's still do it in the Roland way as of now)
 */
#define NPARTS 16

/* Sound bank size equals the number of parts. Also this decision
 * stems from the 4k intro needs. But this is no restriction for more
 * general use either, because, in principle, you could have thousands
 * of patches off-line, and then load one of them for each of the 16
 * channels on-demand. But, for 4k purposes, we expect to need only
 * max 16 patches and 16 channels, and everything will be "hard-coded"
 * (automatically, though, nowadays) at compile time. Adjacent copies
 * of exactly the same patch data should compress nicely, too.
 *
 * Remain in the MIDI world - it is 7 bits per SysEx data bit. Or
 * should we move to our own world altogether? No... SysEx is nice :).
 */


/* Sound structure. Must be consistent with all the parameters! */
#define NENVPERVOICE 6
#define NOSCILLATORS 4
#define NCONTROLLERS 4

/* TODO: Think about the whole envelope madness... use LFOs instead of
 * looping envelopes?
 */
#define TRIGGERSTAGE 6
#define RELEASESTAGE 2
#define LOOPSTAGE 1


/* Length of envelope data block (K1T&L K2T&L K3T&L K4T&L K5T&L) */
#define NENVKNEES 5
#define SYNTI2_NENVD (NENVKNEES*2)
/* TODO: (order of knees might be better reversed ?? 
 * Think about this .. make envs simpler? Probably in some
 * later project..
 */

/* Audio delay storage. Less delays means less computation, faster
 * synth.
 */
#define NDELAYS 8
#define DELAYSAMPLES 0x10000

/* Storage size. TODO: Would be nice to check bounds unless
 * -DULTRASMALL is set.
 */
#define SYNTI2_MAX_SONGBYTES 30000
#define SYNTI2_MAX_SONGEVENTS 15000



#endif
