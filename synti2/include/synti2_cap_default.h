#ifndef SYNTI2_CAPACITIES_INCLUDED
#define SYNTI2_CAPACITIES_INCLUDED
/** @file synti2_cap.h 
 * 
 * OBSERVE: This file is generated by the command line 
 * configurator tool! Editing by hand is not recommended,
 * and neither should it be necessary. See cltool.cxx for
 * more details. Some documentation is given below, though.
 * 
 * This file defines the 'capaities', or limits, of s specific 
 * build of the Synti2 software synthesizer. The idea is that
 * more restricted custom values can be used in compiling the 
 * final 4k target than those that are used while composing the 
 * music and designing patches. The final 4k executable should
 * have only the minimum features necessary for its musical score.
 *
 */

 /* Variables defined here (FIXME: refactor names!!) are:
  * NPARTS - multitimbrality; number of max simultaneous sounds;
  *          also the number of patches in the patch bank
  * NENVPERVOICE - envelopes per voice
  * NOSCILLATORS - oscillators/operators per voice
  * NCONTROLLERS - oscillators/operators per voice
  * NENVKNEES    - number of 'knees' in the envelope
  * NDELAYS      - number of delay lines
  * DELAYSAMPLES - maximum length of each delay line in samples
  * SYNTI2_MAX_SONGBYTES  - maximum size of song sequence data
  * SYNTI2_MAX_SONGEVENTS - maximum number of sequence events
  */

#define NPARTS 16
#define NENVPERVOICE 6
#define NOSCILLATORS 4
#define NCONTROLLERS 4

#define TRIGGERSTAGE 6
#define RELEASESTAGE 2
#define LOOPSTAGE 1

#define NENVKNEES 5
#define SYNTI2_NENVD (NENVKNEES*2)
#define NDELAYS 8
#define DELAYSAMPLES 0x10000
#define SYNTI2_MAX_SONGBYTES 30000
#define SYNTI2_MAX_SONGEVENTS 15000

/* FIXME: not yet implemented.*/
#endif
