#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* For debug prints: */
#include <stdio.h>

#include "synti2.h"
#include "misss.h"


/* Synti2 can be compiled as a Jack-MIDI compatible real-time soft
 * synth. Jack headers are not necessary otherwise...
 */
#ifdef JACK_MIDI
#include "jack/jack.h"
#include "jack/midiport.h"
#include <errno.h>
#endif

/* If any foreign MIDI protocol is used, note-offs should be converted
 * to our own protocol (also a MIDI variant) that note-on with zero
 * velocity means a note-off.
 */
#ifdef JACK_MIDI
#define DO_CONVERT_OFFS
#endif


/* Polyphony */
#define NVOICES 32

/* Multitimbrality */
#define NPARTS 16

#define NOSCILLATORS 4

/* Total number of "counters", i.e., oscillators/operators. */
#define NCOUNTERS (NVOICES * NOSCILLATORS)

/* Multitimbrality */
#define NENVPERVOICE 4

/* Total number of envelopes */
#define NENVS (NVOICES * NENVPERVOICE)

/* Maximum value of the counter type depends on C implementation, so use limits.h */
#define MAX_COUNTER UINT_MAX

/* Number of inner loop iterations (audio frame evaluations) between
 * evaluating "slow-motion stuff" such as MIDI input and
 * envelopes. Must be a divisor of the buffer length. Example: 48000/8
 * would yield 6000 Hz (or faster than midi wire rate) responsiveness.
 * Hmm.. there is an audible problem with "jagged volume" (producing a
 * pitched artefact) if the volume envelope is outside of the inner
 * loop... So I'll keep at least the envelope progression counter code
 * inside. Same must be true for panning which is essentially a volume
 * env. Could it be that pitch or filter envelopes could be outside?
 * TODO: test..
 */
#define NINNERLOOP 8

/* TODO: Think about the whole envelope madness... use LFOs instead of
 * looping envelopes?
 */
#define TRIGGERSTAGE 6

#define WAVETABLE_SIZE 0x10000
#define WAVETABLE_BITMASK 0xffff

/** I finally realized that unsigned ints will nicely loop around
 * (overflow) and as such they model an oscillator's phase pretty
 * nicely. Can I use them for other stuff as well? Seem fit for
 * envelopes with only a little extra (clamping and stop flag). Could
 * I use just one huge counter bank for everything? TODO: Try out, and
 * also think if it is more efficient to put values and deltas in
 * separate arrays. Probably, if you think of a possible assembler
 * implementation with multimedia vector instructions(?).
 */
typedef struct counter {
  unsigned int val;
  unsigned int delta;
} counter;

/** TODO: Not much is inside the part structure. Is it necessary at
 *  all? It will have controller values, though..
 */
typedef struct synti2_part {
  int voiceofkey[128];  /* Which note has triggered which voice*/
} synti2_part;

struct synti2_synth {
  unsigned long sr; /* Better for code size to have indiv. attrib 1st?*/
  float infranotes[128]; /* TODO: This space could be used for LFO's */
  float note2freq[128];  /* pre-computed frequencies of notes... Tuning
			    systems would be easy to change - just
			    compute a different table here (?)..*/
  float ultranotes[128]; /* TODO: This space could be used for noises? */

  float wave[WAVETABLE_SIZE];

  /* Oscillators are now modeled as integer counters (phase). */
  counter c[NCOUNTERS];
  float fc[NCOUNTERS];   /* store the values as floats right away. */

  /* Envelope progression also modeled by integer counters. Not much
   * difference between oscillators and envelopes!!
   */
  counter eprog[NVOICES][NENVPERVOICE];
  float feprog[NVOICES][NENVPERVOICE];   /* store the values as floats right away. */
  float feprev[NVOICES][NENVPERVOICE];   /* previous value (for lin. interp.) */
  float fegoal[NVOICES][NENVPERVOICE];   /* goal value (for lin. interp.) */
  float fenv[NVOICES][NENVPERVOICE];     /* current output (of lin. interp.) */

  /* Envelope stages just a table? TODO: think.*/
  int estage[NVOICES][NENVPERVOICE];
  int sustain[NVOICES];

  int partofvoice[NVOICES];  /* which part has triggered each "voice";
				-1 (should we use zero instead?) means
				that the voice is free to re-occupy. */

  int note[NVOICES];
  int velocity[NVOICES];

  /* The parts. Sixteen as in the MIDI standard. TODO: Could have more? */
  synti2_part part[NPARTS];   /* FIXME: I want to call this channel!!!*/
  float patch[NPARTS * SYNTI2_NPARAMS];  /* The sound parameters per part*/

  /* Throw-away test stuff:*/
  long int global_framesdone;  /*NOTE: This is basically just a counter, too! */

};


#define SYNTI2_MAX_SONGBYTES 30000
#define SYNTI2_MAX_SONGEVENTS 15000

/* Events shall form a singly linked list. TODO: Is the list code too
   complicated? Use just tables with O(n^2) pre-ordering instead?? */
typedef struct synti2_player_ev synti2_player_ev;

struct synti2_player_ev {
  unsigned char *data;
  synti2_player_ev *next;
  unsigned int frame;
  int len;
};

struct synti2_player {
  synti2_player_ev *playloc; /* could we use just one loc for play and ins? */
  synti2_player_ev *insloc;
  int fpt;       /* Frames per tick. Tempos will be inexact, sry! */
  int tpq;          /* Ticks per quarter (no support for SMPTE). */
  synti2_player_ev *freeloc; /*pointer to next free event structure*/
  int frames_done;  /* Runs continuously. Breaks after 12 hrs @ 48000fps !*/
  int sr;           /* Sample rate. */

  int idata;        /* index of next free data location */

  /* Only playable events. The first will be at evpool[0]! */
  synti2_player_ev evpool[SYNTI2_MAX_SONGEVENTS];
  /* Data for events. */
  unsigned char data[SYNTI2_MAX_SONGBYTES];
};

/** Reads a MIDI variable length number. */
static 
int
varlength(const unsigned char * source, int * dest){
  int nread;
  unsigned char byte;
  *dest = 0;
  for (nread=1; nread<=4; nread++){
    byte = *source++;
    *dest += (byte & 0x7f);
    if ((byte & 0x80) == 0)
      return nread; 
    else *dest <<= 7;
  }
  return 0; /* Longer than 4 bytes! Wrong input! FIXME: die. */
}


static
void
synti2_player_reset_insert(synti2_player *pl){
  pl->insloc = pl->evpool; /* The first one. */
}


static
void
synti2_player_event_add(synti2_player *pl, 
                        int frame, 
                        unsigned char *src, 
                        size_t n){
  synti2_player_ev *ev_new;
  //int dcopy;
  unsigned char *dst;
  while((pl->insloc->next != NULL) && (pl->insloc->next->frame <= frame)){
    pl->insloc = pl->insloc->next;
  }
  ev_new = pl->freeloc++;

  /* Fill in the node: */
  ev_new->next = pl->insloc->next;
  ev_new->frame = frame;
  ev_new->len = n;
  pl->insloc->next = ev_new;

  /* Get next available space from the data pool, and copy the data.
   * Using memcpy() seems to yield smaller code than a home-made loop.
   */
  dst = ev_new->data = pl->data + pl->idata;
  memcpy(dst,src,n);  
  pl->idata += n;  
}

/** Returns a pointer to one past end of read in input data, i.e., next byte. */
static
unsigned char *
synti2_player_merge_chunk(synti2_player *pl, 
                          unsigned char *r, 
                          int n_events)
{
  char chan, type;
  int ii;
  int frame, tickdelta;
  unsigned char *par;
  unsigned char tmpbuf[3]; /* see that it gets initialized! */

  chan = *r++;
  type = *r++;
  par = r;
  frame = 0;
  synti2_player_reset_insert(pl); /* Re-start merging from frame 0. */

  //printf("Type %d on chan %d. par 1=%d par 2=%d\n", type, chan, par[0], par[1]);

  /* Always two parameters.. makes reader code simpler with not too
   * much storage overhead.
   */
  r += 2;   /* add number of parameters to r! */

  for(ii=0; ii<n_events; ii++){
    r += varlength(r, &tickdelta);
    frame += pl->fpt * tickdelta;
    //printf("Tickdelta = %d. Frame %d\n", tickdelta, frame);
    //synti2_player_merge_event(pl, );

    if (type <= MISSS_LAYER_NOTES_CVEL_CPITCH){
      tmpbuf[0] = 0x90; /* Assume we're doing notes. */
      tmpbuf[1]= (par[0]==0xff) ? *r++ : par[0];
      tmpbuf[2]= (par[1]==0xff) ? *r++ : par[1];
      /* Now it is a complete msg. */
      synti2_player_event_add(pl, frame, tmpbuf, 3); 
    } else {
      /* Not yet implemented. FIXME: implement? */
      switch (type){
      case MISSS_LAYER_CONTROLLER_RESETS:
      case MISSS_LAYER_CONTROLLER_RAMPS:
      case MISSS_LAYER_SYSEX_OR_ARBITRARY:
      case MISSS_LAYER_NOTHING_AS_OF_YET:
      default:
        break;
      }
    }
  }
  return r;
}

#ifdef JACK_MIDI
/** Creates a future for the player object that will repeat the next
 *  nframes of midi data from a jack audio connection kit midi port.
 *
 * ... If need be.. this could be made into merging function on top of
 * the sequence being played.. but I won't bother for today's
 * purposes...
 */
void
synti2_player_init_from_jack_midi(synti2_player *pl,
                                  jack_port_t *inmidi_port,
                                  jack_nframes_t nframes)
{
  jack_midi_event_t ev;
  jack_nframes_t i, nev;
  void *midi_in_buffer  = (void *) jack_port_get_buffer (inmidi_port, nframes);

  /* Re-initialize, and overwrite any former data. */
  pl->freeloc = pl->evpool+1;
  pl->playloc = pl->evpool; /* This would "rewind" the song */
  pl->insloc = pl->evpool; /* This would "rewind" the song */
  pl->idata = 0;
  pl->playloc->next = NULL; /* This effects the deletion of previous msgs. */
  
  nev = jack_midi_get_event_count(midi_in_buffer);
  for (i=0;i<nev;i++){
    if (jack_midi_event_get (&ev, midi_in_buffer, i) != ENODATA) {

      synti2_player_event_add(pl, 
                              pl->frames_done + ev.time, 
                              ev.buffer, 
                              ev.size);
    } else {
      break;
    }
  }
}
#endif

/** Load and initialize a song. Assumes a freshly created player object!*/
static
void
synti2_player_init_from_misss(synti2_player *pl, unsigned char *r)
{
  int chunksize;
  int uspq;
  /* Initialize an "empty" "head event": */
  pl->freeloc = pl->evpool + 1;

  pl->playloc = pl->evpool; /* This would "rewind" the song */
  
  r += varlength(r, &(pl->tpq));  /* Ticks per quarter note */
  r += varlength(r, &uspq);       /* Microseconds per quarter note */
  pl->fpt = ((float)uspq / pl->tpq) * (pl->sr / 1000000.0); /* frames-per-tick */
  /* TODO: Think about accuracy vs. code size */
  
  r += varlength(r, &chunksize);
  while(chunksize > 0){ /*printf("Read chunksize %d \n", chunksize);*/
    r = synti2_player_merge_chunk(pl, r, chunksize); /* read a chunk... */
    r += varlength(r, &chunksize);                   /* move on to next chunk. */
  }
}


/** Creates a player object. Initial songdata is optional. */
synti2_player *
synti2_player_create(unsigned char * songdata, int samplerate){
  synti2_player *pl;
  
  pl = calloc(1, sizeof(synti2_player));
  
#ifndef ULTRASMALL
  if (pl == NULL) return pl;
#endif
  
  pl->sr = samplerate;
  
  if (songdata != NULL) synti2_player_init_from_misss(pl, songdata);
  
  return pl;
}



/** Allocate and initialize a new synth instance. */
synti2_synth *
synti2_create(unsigned long sr)
{
  synti2_synth * s;
  int ii;

  s = calloc (1, sizeof(synti2_synth));
  if (s == NULL) return s;
  s->sr = sr;

  /* Create a note-to-frequency look-up table (cents need interpolation). 
   * TODO: Which is more efficient / small code: linear interpolation 
   * on every eval or make a lookup table on the cent scale...
   *
   * Idea: I could have negative notes for LFO's and upper octaves to
   * the ultrasound for weird aliasing/noise kinda stuff... just make
   * a bigger table here and let the pitch envelopes go beyond the
   * beyond!
   */
  for(ii=0; ii<128; ii++){
    s->note2freq[ii] = 440.0 * pow(2.0, ((float)ii - 69.0) / 12.0 );
  }

  for(ii=0; ii<NVOICES; ii++){
    s->partofvoice[ii] = -1;     /* Could I make this 0 somehow? */
  }

  for(ii=0; ii<WAVETABLE_SIZE; ii++){
    s->wave[ii] = sin(2*M_PI * ii/(WAVETABLE_SIZE-1));
  }

  return s;
}


/** Note on OR note off (upon velocity == 0) */
static
void
synti2_do_noteon(synti2_synth *s, int part, int note, int vel)
{
  int voice, ie;
  /* note off */
  if (vel==0){
    voice = s->part[part].voiceofkey[note];
    //s->part[part].voiceofkey[note] = -1;  /* key should no longer map to ... no... */
    //if (voice < 0) return; /* FIXME: think.. */
    /* TODO: release all envelopes.. */
    for (ie=0; ie<NENVPERVOICE; ie++){
      s->estage[voice][ie] = 2;      /* skip to end */
      s->eprog[voice][ie].delta = 0; /* skip to end */
      s->eprog[voice][ie].val = 0;   /* must skip also value!! FIXME: think(?)*/
      s->sustain[voice] = 0;         /* don't renew loop. */
    }
    return; /* Note off is now handled. Otherwise do note on. */
  }

 
  /* note on */

  /* FIXME: Unimplemented plan: if patch is monophonic, always use the
   * voice corresponding to the part number. Otherwise, if that
   * primary voice is occupied, find a free voice starting from index
   * 17. This way, there is always one voice available per channel for
   * mono patches, and it will become more deterministic to know where
   * things happen, for possible visualization needs (but there is
   * evil resource wasting when the song uses less than 16 parts!
   * maybe some kind of free voice stack could be implemented with not
   * too much code...)
   */
  for(voice=0; voice < NVOICES; voice++){
    if (s->partofvoice[voice] < 0) break;
  }
  if (voice==NVOICES) return; /* Cannot play new note! */
  /* (Could actually force the last voice to play anyway!?) */
  
  s->part[part].voiceofkey[note] = voice;
  s->partofvoice[voice] = part;
  s->note[voice] = note;
  s->velocity[voice] = vel;
  s->sustain[voice] = 1;  
 
  /* TODO: trigger all envelopes according to patch data..  Just give
     a hint to the evaluator function.. */
  for (ie=0; ie<NENVPERVOICE; ie++){
    s->estage[voice][ie] = TRIGGERSTAGE;
    s->eprog[voice][ie].delta = 0;
  }
}


/** FIXME: Think about data model.. aim at maximal sparsity but enough
    expressive range. If it turns out that the 1/1000 accuracy is very
    seldom required, then it could be worthwhile to store everything
    in the much simpler format of 4*7 = 28-bit signed integer
    representing (decimal) hundredths. Even then most parameters would
    only have the least-significant 7bit set?

    FIXME: If the song sequence data is finally read from SMF, then
    there will already be a subroutine that reads variable length
    values (the time deltas) which could be re-used here. But what
    about accuracy then?

    FIXME: Think about nonlinear parameter range. For example x^2 -
    usually accuracy is critical for the smaller parameter values. But
    no... pitch envelopes need to be accurate on a wide range!!

    SysEx format (planned; FIXME: implement!) 

    F0 00 00 00 [storeAddrMSB] [storeAddr2LSB] [inputLengthMSB] [inputLengthLSB]
    ... data LLSBs... 
    ... data LMSBs... 
    ... data MLSBs... 
    ... data MMSBs... 
    F7

    Length is also the stride for value encoding.
*/
//FIXME (?): static
void
synti2_do_receiveSysEx(synti2_synth *s, unsigned char * data){
  int offset, stride;
  unsigned char adjust_byte, adjust_nib, sign_nib;

  int ir;
  float decoded;

  /* FIXME: Manufacturer ID check.. length check; checksums :) could
   * have checks.. :) but checks are for chicks?
   */

  /* Process header: */
  offset = data[4]; offset <<= 7; offset += data[5];
  stride = data[6]; stride <<= 7; stride += data[7];
  data += 8;
  
  //jack_info("Receiving! Offset %d Length %d", offset, stride);
  /* Process data: */
  for (ir=0; ir<stride; ir++){
    decoded  = (*(data + 0*stride)) * 0.01;  /* hundredths*/
    decoded += (*(data + 1*stride));         /* wholes */
    decoded += (*(data + 2*stride)) * 100.0; /* hundreds*/
    adjust_byte = *(data + 3*stride);
    adjust_nib = adjust_byte & 0x0f;
    sign_nib = adjust_byte >> 4;
    decoded += adjust_nib * 0.001;           /* thousandths*/
    if (sign_nib>0) decoded = -decoded;      /* sign. */

    s->patch[offset++] = decoded;
    data++;
  }
  /* Could check that there is F7 in the end :)*/
}



/** Handles input that comes from the stored list of song events.
 *
 * Renders some frames of control data for the synth, keeping track
 * of song position. This will do the store()s as if the song was
 * played live. The dirty work of figuring out event timing has been
 * done by the song loader, so we just float in here.
 *
 * Upon entry (and all times): pl->next points to the next event to be
 * played. pl->frames_done indicates how far the sequence has been
 * played.
 *
 *  Things may happen late or ahead of time. I don't know if that is
 *  what they call "jitter", but if it is, then this is where I jit
 *  and jat...
 */
static
void
synti2_handleInput(synti2_synth *s, 
                   synti2_player *pl,
                   int upto_frames)
{
  /* TODO: sane implementation */
  unsigned char *midibuf;

  while((pl->playloc->next != NULL) 
        && (pl->playloc->next->frame < upto_frames )) {
    pl->playloc = pl->playloc->next;

    midibuf = pl->playloc->data;
    if ((midibuf[0] & 0xf0) == 0x90){
      synti2_do_noteon(s, midibuf[0] & 0x0f, midibuf[1], midibuf[2]);

#ifdef DO_CONVERT_OFFS
    } else if ((midibuf[0] & 0xf0) == 0x80) {
      /* Convert any note-off to a note-on with velocity 0 here. */
      synti2_do_noteon(s, midibuf[0] & 0x0f, midibuf[1], 0);
#endif
      
    } else if (midibuf[0] == 0xf0){
      synti2_do_receiveSysEx(s, midibuf);
    }else {
      /* Other MIDI messages are silently ignored. */
    }
  }
  pl->frames_done = upto_frames;
}



/** TODO: Implement better? */
static
void
synti2_evalEnvelopes(synti2_synth *s){  
  int ie, iv;
  unsigned int detect;
  /* A counter is triggered just by setting delta positive. */
  /* FIXME: Here could be a good place for some pointer arithmetics
   * instead of this index madness [iv][ie] ... Also see if this and
   * the code for the main oscillators could be combined.
   */
  for(iv=0;iv<NVOICES; iv++){
    for(ie=0;ie<NENVPERVOICE; ie++){
      if (s->eprog[iv][ie].delta == 0) continue;  /* stopped.. not running*/
      detect = s->eprog[iv][ie].val;
      s->eprog[iv][ie].val += s->eprog[iv][ie].delta; /* counts from 0 to MAX */
      if (s->eprog[iv][ie].val < detect){  /* Detect overflow */
        s->eprog[iv][ie].val = 0;    /* FIXME: Is it necessary to reset val? */
        s->eprog[iv][ie].delta = 0;  /* Counter stops after full cycle   */
        s->feprog[iv][ie] = 1.0;     /* and the floating value is clamped */ 
      } else {
        s->feprog[iv][ie] = ((float) s->eprog[iv][ie].val) / MAX_COUNTER;
      }
      s->fenv[iv][ie] = (1.0 - s->feprog[iv][ie]) * s->feprev[iv][ie] 
        + s->feprog[iv][ie] * s->fegoal[iv][ie];
    }
    /* TODO: Observe that for this kind of rotating counter c
       interpreted as x \in [0,1], it is easy to get 1-x since it is
       just -c in the integer domain (?). Useable fact? */
    /* When delta==0, the counter has gone a full cycle and stopped at
     * the maximum floating value, 1.0, and the envelope output stands
     * at the goal value ("set point").
     */
  }
}


/* Advance the oscillator counters. Consider using the xmms assembly
 * for these? Try to combine with the envelope counter code somehow..
 */
static 
void
synti2_evalCounters(synti2_synth *s){
  int ic;
  for(ic=0;ic<NCOUNTERS;ic++){
    s->c[ic].val += s->c[ic].delta;
    s->fc[ic] = (float) s->c[ic].val / MAX_COUNTER;
  }
}


/** Updates envelope stages as the patch data dictates. */
static
void
synti2_updateEnvelopeStages(synti2_synth *s){
  int iv, ie, ipastend;
  int part;
  float nextgoal;
  float nexttime;
  int stagesum;
  
  for(iv=0; iv<NVOICES; iv++){
    part = s->partofvoice[iv];
    if (part<0) continue;
    stagesum = 0;
    for (ie=0; ie<NENVPERVOICE; ie++){
      ipastend = part * SYNTI2_NPARAMS + SYNTI2_IENVS + (ie+1) * SYNTI2_NENVD;
      if (s->estage[iv][ie] == 0) continue; /* skip untriggered envs. */
      /* Think... delta==0 on a triggered envelope means endclamp?? 
         NOTE: Need to set delta=0 upon note on!!
         and estage == NSTAGES+1 or so (=6?) means go to attack.. */
      if (s->eprog[iv][ie].delta == 0){
        s->estage[iv][ie]--; /* Go to next stage (one smaller ind) */
        if (s->estage[iv][ie] == 0) continue; /* Newly ended one.. no more knees. */
        /* Loop happens here (FIXME: make it proper): */
        //if ((s->estage[iv][ie] == 1) && (s->sustain[iv] != 0))
        //  s->estage[iv][ie] = 3;
        nexttime = s->patch[ipastend - s->estage[iv][ie] * 2 + 0];
        nextgoal = s->patch[ipastend - s->estage[iv][ie] * 2 + 1];
        s->feprev[iv][ie] = s->fenv[iv][ie];
        s->fegoal[iv][ie] = nextgoal;
        if (nexttime <= 0.0){
          s->eprog[iv][ie].val = 1; /* hack for "no time". will be 0 after an eval. */
          s->eprog[iv][ie].delta = MAX_COUNTER;
        } else {
          s->eprog[iv][ie].delta = MAX_COUNTER / s->sr / nexttime;
        }
        /*
          if (ie==0)
          jack_info("v%02de%02d(Rx%02d): stage %d at %.2f to %.2f in %.2fs (d=%d) ", 
          iv, ie, part, s->estage[iv][ie], s->feprev[iv][ie], 
          s->fegoal[iv][ie], 
          nexttime, s->eprog[iv][ie].delta);
        */
      }
      /* FIXME: just simple LFO stuff instead of this hack of a looping envelope?*/
      /* FIXME: Or make looping optional by a simple sound parameter... */
      stagesum += s->estage[iv][ie];
    }
    if (stagesum == 0){
      /* Consider note instance "completely finished" when all envelopes are over: */
      s->partofvoice[iv] = -1;
    }
  }
}


/** Converts note values to counter deltas. */
static
void
synti2_updateFrequencies(synti2_synth *s){
  int iv, note;
  float notemod, interm, freq;
  /* Frequency computation... where to put it after all? */
  for (iv=0; iv<NVOICES; iv++){
    notemod = s->note[iv] + s->fenv[iv][2];   // HACK!!
    /* should make a floor (does it? check spec)*/
    note = notemod;
    interm = (1.0 + 0.05946 * (notemod - note)); /* +cents.. */
    freq = interm * s->note2freq[note];
    
    s->c[iv*2].delta = freq / s->sr * MAX_COUNTER;
    
    /* modulator pitch; Hack. FIXME: */
    notemod = s->note[iv] + s->fenv[iv][3];   // HACK!!
    /* should make a floor (does it? check spec)*/
    note = notemod;
    interm = (1.0 + 0.05946 * (notemod - note)); /* +cents.. */
    freq = interm * s->note2freq[note];
    
    s->c[iv*2+1].delta = (freq) / s->sr * MAX_COUNTER; /*hack test*/
    //s->c[2].delta = freq / s->sr * MAX_COUNTER; /* hack test */
  }
}


void
synti2_render(synti2_synth *s,
              synti2_player *pl,
              synti2_smp_t *buffer,
              int nframes)
{
  int iframe, ii, iv;
  float interm;
  
  for (iframe=0; iframe<nframes; iframe += NINNERLOOP){
    /* Outer loop for things that are allowed some jitter. (further
     * elaboration in comments near the definition of NINNERLOOP).
     */
    
    /* Handle MIDI-ish controls if there are more of them waiting. */
    synti2_handleInput(s, pl, s->global_framesdone + iframe + NINNERLOOP);
    synti2_updateEnvelopeStages(s); /* move on if need be. */
    synti2_updateFrequencies(s);    /* frequency upd. */
    
    /* Inner loop runs the oscillators and audio generation. */
    for(ii=0;ii<NINNERLOOP;ii++){
      
      /* Could I put all counter upds in one big awesomely parallel loop? */
      synti2_evalEnvelopes(s);
      synti2_evalCounters(s);
      
      /* Getting more realistic soon: */
      buffer[iframe+ii] = 0.0;
      
      for(iv=0;iv<NVOICES;iv++){
        // if (s->partofvoice[iv] < 0) continue; /* Unsounding. FIXME: (f1) */
        /* Produce sound :) First modulator, then carrier*/
        /* Worth using a wavetable? Definitely.. Could do the first just
           by bit-shifting the counter... */
        interm  = s->wave[(unsigned int)(s->fc[iv*2+1] * WAVETABLE_SIZE) & WAVETABLE_BITMASK];
        interm *= interm * interm; /* Hack!! BEAUTIFUL!!*/
        interm *= (s->velocity[iv]/128.0) * (s->fenv[iv][1]);
        interm  = s->wave[(unsigned int)((s->fc[iv*2+0] + interm) * WAVETABLE_SIZE) & WAVETABLE_BITMASK];
        interm *= s->fenv[iv][0];
        buffer[iframe+ii] += interm;
      }      
      buffer[iframe+ii] /= NVOICES;
      
      //buffer[iframe+ii] = sin(32*buffer[iframe+ii]); /* Hack! beautiful too!*/
      //buffer[iframe+ii] = tanh(16*buffer[iframe+ii]); /* Hack! beautiful too!*/
    }
  }
  s->global_framesdone += nframes;
}
