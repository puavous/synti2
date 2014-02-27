/** Universally constant (maximum/limiting) values for synti2 software
 *  synthesizer.
 */
 
#ifndef SYNTI2_LIMITS_H
#define SYNTI2_LIMITS_H

/* Arbitrary limits on operators, modulators and voices. */
#define NUM_MAX_OPERATORS 6
#define NUM_MAX_MODULATORS 4
#define NUM_MAX_CHANNELS 256

/* Minimums. Always need 2 knees in 1 envelope to hear any sound. */
#define NUM_MIN_KNEES 2
#define NUM_MIN_ENVS 1

/* Arbitrary limits on data size. */
#define SYNTI2_DELAYSAMPLES 0x10000
#define SYNTI2_MAX_SONGBYTES 30000
#define SYNTI2_MAX_SONGEVENTS 15000

#endif
