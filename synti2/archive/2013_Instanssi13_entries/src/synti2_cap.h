#ifndef SYNTI2_CAPACITIES_INCLUDED
#define SYNTI2_CAPACITIES_INCLUDED

#define NPARTS 16

/* Sound structure. Must be consistent with all the parameters! */
#define NENVPERVOICE 2
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
#define NDELAYS 4
#define DELAYSAMPLES 0x10000

/* Storage size. TODO: Would be nice to check bounds unless
 * -DULTRASMALL is set.
 */
#define SYNTI2_MAX_SONGBYTES 30000
#define SYNTI2_MAX_SONGEVENTS 15000



#endif
