/** MISSS - Midi-like interface for Synti2 Software Synthesizer.
 *
 * This program reads and analyzes a standard MIDI file (type 0 or
 * type 1), tries to mutilate it in various ways to reduce its size,
 * and outputs a file compatible with the super-simplistic sequencer
 * part of the Synti2 Software Synthesizer.
 *
 * FIXME: I want to redo this with some more flexible programming
 * language..
 *
 * Ideas:
 *
 * + Trash all events that Synti2 doesn't handle.
 *
 * + Use simplistic note-off (not going to use note-off velocity)
 *
 * + Make exactly one "track" - call it "layer"! - per one MIDI status
 *   word, use the channel nibble of the status byte for something
 *   more useful (which is? Could be "chord size", i.e., number of
 *   instantaneous notes, no need for zero delta!), and write
 *   everything as "running status". Make a very simple layering
 *   algorithm to piece it together upon sequencer creation. Just "for
 *   layer in layers do song.addlayer(layer)" and then start
 *   playing. Will be very simple at that point.
 *
 * + Trash header information that Synti2 doesn't need.
 *
 * + Only one BPM value allowed throughout the song; no tempo map,
 *   sry. (No need to compute much during the playback).
 *
 * + Use lower parts-per-quarter setting, and quantize notes as
 *   needed. The variable-length delta idea is useful as it is.
 *   FIXME: Could I re-use the var-length decoder for the patches???
 *   I think I could, somehow... if I just figured out how...
 *
 * - Decimate velocities
 *
 * - Approximate controllers with linear ramps.
 *
 * - Turn percussion tracks to binary beat patterns(?).
 *
 * - Observe: Percussive instruments need no note-off.
 *
 */

#include<stdlib.h>
#include<stdio.h>
#include<getopt.h>
#include<string.h>

#include<math.h>

#include "synti2_misss.h"

#define MAX_NODES 0x10000
#define MAX_DATA 0x1000000
#define MAX_STRINGLENGTH 200

/* -- linked list of timed events (easy to add, merge, and iterate
      linearly from beginning to end) -- */
/* Plan this here; move to a separate unit soon..*/

typedef struct evlist_node evlist_node;

struct evlist_node{
  unsigned int tick;
  void *data;
  int datalen;
  evlist_node *next;
  /* TODO(?): int orig_track; // from which track */
  /* TODO(?): int orig_seqid; // on which designated sequence (FF 00 02 ...) */
};

/** Event layers (complete deconstruction). Linear pool of nodes. Deep
 *  copy of data bytes.
 */
typedef struct smf_events{
  evlist_node nodepool[MAX_NODES];
  unsigned char datapool[MAX_DATA];
  int next_free_node;
  int next_free_data;
  
  int n_total;
  int n_stored;
  int n_unknown; /*hmm... let's see.. maybe there won't be any use for this(?)*/

  unsigned char last_status; /* Needed when a track contains running
                      status information; must read from one track at
                      a time. */
  unsigned char last_par1; /* For example note or CC#*/
  /* One list for each possible two-byte combination (status byte and
   * first parameter)... We'll be doing a maximal filtering job later
   * on.. maybe?
   */
  evlist_node head[0x10000];  /* Zero init -> tick 0, null data, null next */
  evlist_node *iter[0x10000];
} smf_events;

/** MIDI-like Interface for Synti2 Software Synthesizer, MISSS.
 *
 * Event layers. Linear pool of nodes. Data pointers can be data(?)
 *
 * Just a data string? Or should it in fact be a C-compilable ASCII
 * string already?
 */
typedef struct misss_events{
  unsigned char datapool[MAX_DATA];
  int ind;
} misss_events;

/* TODO(?): put into its own module? */
typedef struct misss_info{
  int tempo; /* FIXME: what would be a suitable unit? */
} misss_info;

#if 0

       Blaaa.. it would be very similar to smf_events... should combine somehow..

typedef struct misss_events{
  evlist_node nodepool[MAX_NODES];
  unsigned char datapool[MAX_DATA];
  int next_free_node;
  int next_free_data;

} misss_events;
#endif



/* Options for MIDI mutilation. */
typedef struct misssify_options{
  char infname[MAX_STRINGLENGTH+1];
  char outfname[MAX_STRINGLENGTH+1];
  int verbose;
  int options_valid;
  int desired_timediv; /* desired value of timediv */
  int override_all_velocities; /*0 or common velocity for every note on. */
  int trash_all_noteoffs; /* if only note-ons suffice for everything.. */
} misssify_options;


typedef struct timesig_t{
  unsigned char numerator;
  unsigned char denominator; /* as a power of two */
  unsigned char metroclocks;
  unsigned char notation32nds;
} timesig_t;

/* Information about the midi file and the process. */
typedef struct smf_info{
  int ntracks_total;
  int ntracks_read;
  int ntracks_alien;
  int smf_format;
  int time_division;

  int usec_per_q;
  timesig_t timesig;
  int tempo_initialized;
  int timesig_initialized;
} smf_info;



unsigned char dinput[MAX_DATA];
int dinput_size;
unsigned char dmidi[MAX_DATA];
unsigned char dmisss[MAX_DATA];
int dmisss_size;



/** Interprets command line.
 *
 * TODO: Implement this; so far only example code from the GNU manual.
 *
 * TODO (later): learn to use argp instead of getopt..
 */
static
void
misssify_options_parse(misssify_options *opt, int argc, char *argv[]){
  int c;
  static int local_verbose; /* must know address on compile time. */
  /* Defaults for some values:*/
  strncpy(opt->outfname, "test.misss", MAX_STRINGLENGTH);
  /* Aah, but this will only fail possibly for user input:
  if (strlen(str) >= MAX_STRINGLENGTH){
    fprintf(stderr, "File path too long. %s", str);
    exit(0);
  }
  */
  opt->infname[0]='\0';

  while (1){
    static struct option long_options[] =
      {
        /* These options set a flag. */
        {"verbose", no_argument,       &local_verbose, 1},
        {"brief",   no_argument,       &local_verbose, 0},
        /* These options don't set a flag.
           We distinguish them by their indices. */
        {"add",     no_argument,       0, 'a'},
        {"append",  no_argument,       0, 'b'},
        {"delete",  required_argument, 0, 'd'},
        {"velocity-all",  required_argument, 0, 'l'},
        {"only-on", no_argument, 0, 'o'},
        {"tpq",     required_argument, 0, 'p'},
        {0, 0, 0, 0} /* marks the end of options */
      };
    /* getopt_long stores the option index here. */
    int option_index = 0;
    
    c = getopt_long (argc, argv, "ab:d:l:op:",
                     long_options, &option_index);
    
    /* Detect the end of the options. */
    if (c == -1)
      break;
    
    switch (c)
      {
      case 0:
        /* If this option set a flag, do nothing else now. */
        if (long_options[option_index].flag != 0)
          break;
        printf ("option %s", long_options[option_index].name);
        if (optarg)
          printf (" with arg %s", optarg);
        printf ("\n");
        break;
        
      case 'a':
        puts ("option -a\n");
        break;
        
      case 'b':
        puts ("option -b\n");
        break;
        
      case 'c':
        printf ("option -c with value `%s'\n", optarg);
        break;
        
      case 'd':
        printf ("option -d with value `%s'\n", optarg);
        break;

      case 'l':
        opt->override_all_velocities = atoi(optarg);
        printf ("All velocities will be %d (given as string '%s')\n", 
                opt->override_all_velocities, optarg);
        break;

      case 'o':
        opt->trash_all_noteoffs = 1;
        printf ("No note-offs will be written on *any* part. \n");
        break;
        
      case 'p':
        opt->desired_timediv = atoi(optarg);
        printf ("Desired timediv is %d (given as string '%s')\n", 
                opt->desired_timediv, optarg);
        break;
        
      case '?':
        /* getopt_long already printed an error message. */
        break;
        
      default:
        abort ();
      }
  }
  
  /* Instead of reporting ‘--verbose’
     and ‘--brief’ as they are encountered,
     we report the final status resulting from them. */
  opt->verbose = local_verbose;
  if (opt->verbose){
    fprintf(stderr, "Verbose on. I'll start talking to stderr.\n");
  }
  
  /* Remaining command line arguments (not options); we must have
     input file name. */
  if (optind >= argc){
    fprintf(stderr, "Input file name must be given!\n");
    exit(0);
  }
  if (strlen(argv[optind]) >= MAX_STRINGLENGTH){
    fprintf(stderr, "Output file path too long. Will use default! %s\n", argv[argc]);
  } else {
    strncpy(opt->infname, argv[optind], MAX_STRINGLENGTH);
  }
}



/** Reads a number of 'bytecount' bytes from MIDI data. */
static 
unsigned int
smf_read_int(const unsigned char * source, int bytecount){
  int i, res;
  res = 0;
  for (i=0;i<bytecount;i++){
    res <<= 8;
    res += source[i];
  }
  return res;
}



/** Reads a MIDI variable length number. Stores the value into the
 *   destination given as a pointer.  Returns number of bytes read.
 *
 * FIXME: In C you can return structs, right? Then should return
 * {value, len} -pairs!
 */
static 
int
smf_read_varlength(const unsigned char * source, int * dest){
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
  fprintf(stderr,
          "A varlength number longer than 4 bytes. Something is wrong; sorry.\n"); 
  exit(2);
  return 0; 
}



/** Extended status contains status byte and first data byte. */
static
void
smf_events_iter_rewind(smf_events *evs, int extended_status){
  evs->iter[extended_status] = evs->head + extended_status;
}

static
void
smf_events_rewind_all_iters(smf_events *evs){
  int i;
  for (i=0;i<0x10000; i++){
    smf_events_iter_rewind(evs, i);
  }
}

static int smf_events_iter_past_end(smf_events *evs, int extended_status){
  return evs->iter[extended_status] == NULL;
}

static void smf_events_iter_next(smf_events *evs, int extended_status){
  evs->iter[extended_status] = evs->iter[extended_status]->next;
}

static
void
smf_events_iter_tofirst(smf_events *evs, int extended_status){
  smf_events_iter_rewind(evs, extended_status);
  smf_events_iter_next(evs, extended_status);
}

static int smf_events_get_next_tick(smf_events *evs, int extended_status){
  if (evs->iter[extended_status]->next == NULL) return 0;
  else return evs->iter[extended_status]->next->tick;
}

static int smf_events_get_current_tick(smf_events *evs, int extended_status){
  return evs->iter[extended_status]->tick;
}


static
void
smf_events_skip_all_before_tick(smf_events *evs, int extended_status, int tick){
  while((evs->iter[extended_status]->next != NULL)
        && (smf_events_get_next_tick(evs, extended_status) < tick)){
    smf_events_iter_next(evs, extended_status);
  }
}

static
void
smf_events_printcontents(smf_events *evs, FILE *fto){
  int i;
  evlist_node *p;
  for (i=0x0000; i<0x10000; i++){
    if ((p = evs->head[i].next) != NULL){
      fprintf(fto, "Events %04x at ticks:", i);
      for (;p != NULL; p=p->next){
        fprintf(fto, " %d", p->tick);
      }
      fprintf(fto, "\n");
    }
  }
}

static
void
smf_events_recompute_ticks(smf_events *evs, 
                           int timediv_old,
                           int timediv_new)
{
  float ntick, tdiv;
  int i;
  evlist_node *p;

  tdiv = (float) timediv_old / timediv_new;
  for (i=0x0000; i<0x10000; i++){
    for (p = evs->head[i].next; p != NULL; p=p->next){
      ntick = p->tick / tdiv;
      ntick = floor(ntick + 0.5); /* to closest possible tick.. */
      p->tick = ntick;
    }
  }
}


/* FIXME: Mein gott.. these would be so easy to do as filter
 * and map functions.. 
 */
static
void
smf_events_reset_all_velocities(smf_events *evs, int vel)
{
  int i;
  evlist_node *p;

  for (i=0x9000; i<0x9fff; i++){ /*see.. there's a filter */
    for (p = evs->head[i].next; p != NULL; p=p->next){
      ((unsigned char*)(p->data))[2]=vel; /*.. and there's a map. */ 
      /*and we're in a list. */
    }
  }
}



/* plaah smf_events_merge_at_iter(smf_events *evs,
                         unsigned char *data,
                         int datalen,
                         unsigned int tick); */


/* Merges a midi event to its appropriate location. Must store events
 * in chronological order after rewinding evs. Datalen must be the
 * number of bytes that follow the status byte (the byte itself
 * excluded from the count).
 */
static
void
smf_events_merge_new(smf_events *evs,
                     int extended_status,
                     unsigned char *data,
                     int datalen,
                     unsigned int tick)
{
  evlist_node *newnode;

  if (tick < (evs->iter[extended_status])->tick){
    fprintf(stderr, "Event ordering error. %d < %d Skipping event!\n", 
            tick, evs->iter[extended_status]->tick);
    return;
  }
  
  if (MAX_DATA - evs->next_free_data < (datalen)){
    fprintf(stderr, "Out of data space. Recompile me with larger constants.");
    exit(32);
  }

  if (evs->next_free_node == MAX_NODES){
    fprintf(stderr, "Out of node space. Recompile me with larger constants.");
    exit(32);
  }

  newnode = &(evs->nodepool[evs->next_free_node++]);
  newnode->tick = tick;
  newnode->datalen = datalen;

  /* if there is just 1 byte of data, it is already in
     extended_status, in which case storelen is 0 and nothing gets
     copied. */
  if (datalen > 0){
    newnode->data = &(evs->datapool[evs->next_free_data]);
    evs->next_free_data += (datalen);
    memcpy(newnode->data, data, datalen);
  } else {
    newnode->data = NULL;
  }

  /* Find insert location. */
  smf_events_skip_all_before_tick(evs, extended_status, tick);

  /* Iterator now points to node after which the insertion is to be made. */
  newnode->next = evs->iter[extended_status]->next;
  evs->iter[extended_status]->next = newnode;
  smf_events_iter_next(evs, extended_status); /* don't reverse order */
  evs->n_stored++;
}



/** Merges a list from ev_from with a list from ev_to; result remains in ev_to.*/
static
void
smf_events_merge_lists(smf_events *ev_from, smf_events *ev_to, int s_from, int s_to){
  int tickF;
  smf_events_iter_rewind(ev_to, s_to);
  
  for(smf_events_iter_tofirst(ev_from, s_from);
      !smf_events_iter_past_end(ev_from, s_from);
      smf_events_iter_next(ev_from, s_from)) {
    tickF = smf_events_get_current_tick(ev_from, s_from);
    
    smf_events_skip_all_before_tick(ev_to, s_to, tickF);
    smf_events_merge_new(ev_to, s_to, 
                         ev_from->iter[s_from]->data, 
                         ev_from->iter[s_from]->datalen, 
                         tickF);
  } 
}


static
int
smf_event_contents_from_midi(smf_events *evs, 
                             unsigned char *data, 
                             unsigned int tick)
{
  int vlval, vllen;
  int extended_status;
  extended_status = evs->last_status * 0x100 + (*data);

  switch(evs->last_status >> 4){
  case 0:
    fprintf(stderr, "Looks like a bug; sorry.\n"); /*assert status>0, actually.*/
    exit(1);
  case 8: case 9: case 0xa: case 0xb:
    smf_events_merge_new(evs,extended_status,data-1,3,tick); return 2;
  case 0xc: case 0xd:
    smf_events_merge_new(evs,extended_status,data-1,2,tick); return 1;
  case 0xe:
    smf_events_merge_new(evs,extended_status,data-1,3,tick); return 2;
  }
  /* Now we know that the status is actually either SysEx or Meta*/
  if ((evs->last_status == 0xF0) || evs->last_status == 0xF7){
    /* SysEx FIXME: test... */
    fprintf(stderr, "SysEx feature is not yet tested; sorry\n");
    vllen = smf_read_varlength(data, &vlval);
    extended_status = evs->last_status * 0x100 + (*(data-1));
    smf_events_merge_new(evs,extended_status,data-2,vlval+2,tick); /* tweak to save whole length. */
    return vllen + vlval;
  } else if (evs->last_status == 0xFF) {
    /* Meta event. Just a long batch of whatever; the 'extended
     * status' will contain the meta event type. FIXME: Test. */
    vllen = smf_read_varlength(data+1, &vlval);
    smf_events_merge_new(evs,extended_status,data-1,vlval+2,tick);
    return 1 + vllen + vlval;
  } else {
    fprintf(stderr, "Status 0x%x Looks like a bug; sorry.\n",evs->last_status);
    exit(1);
  }
}



static
int
smf_events_event_from_midi(smf_events *evs, unsigned char *data, int tick){
  if(data[0] >= 0x80){
    /* Complete status, update the status field. */
    evs->last_status = data[0];
    return 1 + smf_event_contents_from_midi(evs, data+1, tick);
  }
  /* Data may continue also with previous "running" status. */
  return smf_event_contents_from_midi(evs, data, tick);
}



static
int
deconstruct_track(smf_events *evs, 
                  smf_info *info,
                  unsigned char *dinput, 
                  misssify_options *opt)
{
  int chunk_size = 0;
  int iread = 0;
  int time_delta = 0;
  int time_actual = 0;
  chunk_size = smf_read_int(dinput+4, 4);
  if (memcmp(dinput,"MTrk",4) != 0) {
    if (opt->verbose){
      fprintf(stderr, "Alien chunk type (\"%4s\"). Skipping\n", dinput);
    }
    info->ntracks_alien++;
    return chunk_size;
  }
  if (opt->verbose)
    fprintf(stderr, "Chunk type (\"%4s\"). Length %d \n", dinput, chunk_size);
  dinput += 8; /* Move past type and size, to first event (which must exist) */

  smf_events_rewind_all_iters(evs);

  /*chunk_size = read_a_track_chunk();*/
  iread=0;
  for(;;){
    if(iread>chunk_size){
      fprintf(stderr, 
              "Invalid chunk: data beyond the reported size (%d) \n",
              chunk_size);
      return chunk_size;
    }
    iread += smf_read_varlength(&(dinput[iread]), &time_delta);
    time_actual += time_delta;
    evs->n_total++;
    if ((dinput[iread] == 0xff) 
        && (dinput[iread+1] == 0x2f) 
        && (dinput[iread+2] == 0x00)){
      if (opt->verbose){
        fprintf(stderr, 
                "Normal end of track. Stored %d events out of %d total.\n",
                evs->n_stored, evs->n_total);
      }
      break;
    }
    /*printf("%d at tick %d : Next up: 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x\n", 
           iread, time_actual, dinput[iread], dinput[iread+1], dinput[iread+2], 
           dinput[iread+3], dinput[iread+4], dinput[iread+5], dinput[iread+6]);
    */
    iread += smf_events_event_from_midi(evs, &(dinput[iread]), time_actual);
  }
  return chunk_size;
}



static
void
deconstruct_from_midi(smf_events *ev_original, 
                      smf_info *info,
                      unsigned char *dinput, 
                      misssify_options *opt)
{
  /* TODO: these to a struct(?): FIXME: Yes, put these to a struct:*/
  int chunk_size = 0;
  int itrack = 0;
  evlist_node *p;
  timesig_t deftimesig = {4,2,24,24};

  /* Verify MIDI Header */
  if (memcmp(dinput,"MThd",4) != 0) {
    fprintf(stderr, "Data doesn't begin with SMF header! Exiting.\n"); exit(1);
  }
  dinput += 4;
  chunk_size = smf_read_int(dinput, 4); dinput += 4;
  if (chunk_size != 6) {
    fprintf(stderr, "Header chunk length unexpected (%d).", chunk_size);
  }
  info->smf_format = smf_read_int(dinput,2); dinput += 2;
  info->ntracks_total = smf_read_int(dinput,2); dinput += 2;
  info->time_division = smf_read_int(dinput,2); dinput += 2;
  if (opt->verbose){
    fprintf(stderr, 
            "SMF header: format %d ; ntracks %d ; time_division 0x%04x (=%d)\n",
            info->smf_format, info->ntracks_total, info->time_division, 
            info->time_division);
    fprintf(stderr,
            "I'm not a MIDI validator, so I'm not checking file consistency!\n");
  }
  if (info->smf_format > 1) {
    fprintf(stderr,
            "Midi file is Type %d but only Type 0 and 1 are supported, sorry.\n",
            info->smf_format);
    exit(1);
  }
  if (info->time_division & 0x8000) {
    fprintf(stderr,
            "Midi file is in SMPTE time format, which is not supported, sorry.\n");
    exit(1);
  }
  dinput += (chunk_size - 6); /* Spec says the header 'could' be longer.*/

  /* TODO: For a more complete reader, this could be a proper time to
   * read tempo map... (or later... think?). But Synti2 shall not be
   * burdened with tempo changes, so that'll be a task for some later
   * project...
   */
  for (itrack=0; itrack < info->ntracks_total; itrack++){
    if (opt->verbose){
      fprintf(stderr, "Processing track %d/%d (index=%d)\n", 
              itrack+1, info->ntracks_total, itrack);
    }
    chunk_size = deconstruct_track(ev_original, info, dinput, opt);
    dinput += 4 + 4 + chunk_size; /*<type(4b)><length(4b)><event(chunk_size)>+*/
  }

  /* A polite software would handle FF 20 (MIDI channel prefix) or
   * warn if any of those have been omitted..
   */

  /* We'll just look for the first events describing tempo and time signature. */
  if ((p = ev_original->head[0xff58].next) != NULL){
    if (opt->verbose) fprintf(stderr, "Using first timesig. \n");
    info->timesig = *((timesig_t*)((unsigned char *)p->data+1));
    info->timesig_initialized = 1;
  } else {
    info->timesig = deftimesig;
  }
  if ((p=ev_original->head[0xff51].next) != NULL){
    if (opt->verbose) fprintf(stderr, "Using first tempo. \n");
    info->usec_per_q = smf_read_int((unsigned char *)p->data+1, 3);
    info->tempo_initialized = 1;
  } else {
    info->usec_per_q = 500000;
  }
  if ((info->timesig_initialized != 1) || (info->timesig_initialized != 1)){
    if (opt->verbose){
      fprintf(stderr, "Defaults used in timing information. \n");
    }
  }
  if (opt->verbose){
    fprintf(stderr,
            "Timesig %d/(2^%d)   usecpq %d\n", 
            info->timesig.numerator, info->timesig.denominator,
            info->usec_per_q);
  } 
}

/* For simplicity, use the varlength of SMF.*/
static
int
misss_encode_varlength(int value, unsigned char *dest){
  unsigned char bytes[4];
  int i, vllen;
  /* to 7 bit pieces: */
  bytes[0] = (value >> 21) & 0x7f;
  bytes[1] = (value >> 14) & 0x7f;
  bytes[2] = (value >> 7) & 0x7f;
  bytes[3] = (value >> 0) & 0x7f;
  /* Set the continuation bits where needed: */
  vllen = 0;
  for(i=0;i<3;i++){
    if ((vllen > 0) || (bytes[i] != 0)) vllen += 1;
    if (vllen > 0) *(dest++) = bytes[i] | 0x80;
  }
  vllen += 1; 
  *(dest++) = bytes[3];
  return vllen; 
}

static
void
misss_events_write_bytes(misss_events *ev_misss, 
                         int length, 
                         const unsigned char *source)
{
  memcpy(&(ev_misss->datapool[ev_misss->ind]), source, length);
  ev_misss->ind += length;
}


static
void
misss_events_write_byte(misss_events *ev_misss, unsigned char byte){
  ev_misss->datapool[ev_misss->ind++] = byte;
}

static
void
misss_events_write_varlength(misss_events *ev_misss, int data){
  ev_misss->ind += misss_encode_varlength(data,&(ev_misss->datapool[ev_misss->ind]));
}



static
void
misss_events_write_layer_header(misss_events *ev_misss, 
                                int number_of_events,
                                int layer_type,
                                int layer_channel){
  misss_events_write_varlength(ev_misss, number_of_events);
  misss_events_write_byte(ev_misss, layer_channel);
  misss_events_write_byte(ev_misss, layer_type);
}

static
void
misss_events_write_header(misss_events *ev_misss, smf_info *info){
  misss_events_write_varlength(ev_misss, info->time_division);
  misss_events_write_varlength(ev_misss, info->usec_per_q);
}


static
void
misss_events_write_notestuff(misss_events *ev_misss, 
                             smf_events *ev_from, 
                             int s_from,
                             int default_pitch,
                             int default_velocity){
  unsigned char *tmpbuf;
  int i, nev, tick_prev, tick_now, tick_delta;
  int chan = 0;

  tmpbuf = malloc(MAX_DATA * sizeof(unsigned char));

  i = 0; 
  nev = 0;
  tick_prev = 0;

  for(smf_events_iter_tofirst(ev_from, s_from);
      !smf_events_iter_past_end(ev_from, s_from);
      smf_events_iter_next(ev_from, s_from)) {
    tick_now = smf_events_get_current_tick(ev_from, s_from);
    tick_delta = tick_now - tick_prev;
    tick_prev = tick_now;

    i += misss_encode_varlength(tick_delta, &(tmpbuf[i]));

    nev++;
    
    /* FIXME: method for digMidiByte(...)*/

    if (default_pitch < 0)
      tmpbuf[i++] = ((unsigned char*)(ev_from->iter[s_from]->data))[1]; /* Note */
    
    if (default_velocity < 0)
      tmpbuf[i++] = ((unsigned char*)(ev_from->iter[s_from]->data))[2]; /* Velocity */
  } 
  /* FIXME: Can only do channel 0 as of yet: */
  /* FIXME: I use -1 (==0xff) in the header as a marker for "no
   *   parameter".  This might imply some things. Check if this is
   *   OK. It seems to make the reader code more straightforward. AND
   *   it looks like we're ending up with only one kind of note
   *   layer!!!
   */
  if ((default_pitch >= 0) && (default_velocity >= 0)){
    misss_events_write_layer_header(ev_misss,nev, MISSS_LAYER_NOTES_CVEL_CPITCH, chan);
    misss_events_write_byte(ev_misss, default_pitch);    /*Pitch 1st*/
    misss_events_write_byte(ev_misss, default_velocity); /*Velocity 2nd*/
  } else if (default_velocity >= 0){
    misss_events_write_layer_header(ev_misss, nev, MISSS_LAYER_NOTES_CVEL, chan);
    misss_events_write_byte(ev_misss, -1); /* empty */
    misss_events_write_byte(ev_misss, default_velocity); /* Vel 2nd */
  } else if (default_pitch >= 0){
    misss_events_write_layer_header(ev_misss, nev, MISSS_LAYER_NOTES_CPITCH, chan);
    misss_events_write_byte(ev_misss, default_pitch); /* Pitch 1st */
    misss_events_write_byte(ev_misss, -1); /* empty */
  } else {
    misss_events_write_layer_header(ev_misss, nev, MISSS_LAYER_NOTES, chan);
    misss_events_write_byte(ev_misss, -1); /* empty */
    misss_events_write_byte(ev_misss, -1); /* empty */
  }
  /* And then the rest of the data. */
  misss_events_write_bytes(ev_misss, i, tmpbuf);

  free(tmpbuf);
}


/* Assumes a fresh ev_misss */
static
void
construct_misss(smf_events *ev_original, 
                misss_events *ev_misss, 
                smf_info *info, 
                misssify_options *opt)
{
  int ilayer;
  int ichan, inote;
  int s_from, s_noteon_to, s_noteoff_to;
  smf_events *ev_intermediate;
  ev_intermediate = calloc(1, sizeof(smf_events));

  /*smf_events_rewind_all_iters(ev_original);*/
  ilayer = 0; /* Store to adjacent layers starting from 0?*/
  /* FIXME: preliminary as of yet. */
  /* Merge note-ons. FIXME: Channel per layer */

  for(ichan=0;ichan<15;ichan++){
    s_noteon_to = ichan;
    s_noteoff_to = 16+ichan;
    for(inote=0;inote<128;inote++){
      /* FIXME: make this proper. Organize the intermediate store... */
      s_from = (0x90 + ichan) * 0x100 + inote;
      smf_events_merge_lists(ev_original, ev_intermediate, s_from, s_noteon_to);

      s_from = (0x80 + ichan) * 0x100 + inote;
      smf_events_merge_lists(ev_original, ev_intermediate, s_from, s_noteoff_to);
    }
  }

  /* Handle global wishes */
  if (opt->desired_timediv > 0){
    smf_events_recompute_ticks(ev_intermediate, 
                               info->time_division,
                               opt->desired_timediv);
    info->time_division = opt->desired_timediv;
  }

  if (opt->override_all_velocities > 0){
    smf_events_reset_all_velocities(ev_intermediate, 
                                    opt->override_all_velocities);
  }


  /*smf_events_printcontents(ev_intermediate, stdout);*/
  misss_events_write_header(ev_misss, info);
  if (opt->override_all_velocities > 0){
    misss_events_write_notestuff(ev_misss, ev_intermediate, 0x0000, -1, 
                                 opt->override_all_velocities);
  } else {
    misss_events_write_notestuff(ev_misss, ev_intermediate, 0x0000, -1, -1);
  }
  if (opt->trash_all_noteoffs == 0){
    misss_events_write_notestuff(ev_misss, ev_intermediate, 0x0010, -1, 0);
  }
  misss_events_write_byte(ev_misss, 0); /* Zero length indicates end of file. */
  free(ev_intermediate);
}



static
int
file_read(const char fname[], unsigned char *buf, int bufsz){
  FILE *f;
  int count;
  f = fopen(fname, "r");
  count = fread(buf, 1, bufsz, f);
  if (count==bufsz){
    fprintf(stderr, "Buffer full while reading file %s\n", fname);
    fclose(f);
    exit(0);
  }
  fclose(f);
  return count;
}



static
void
file_write(const char fname[], const unsigned char *buf, size_t bufsz){
  FILE *f;
  /* FIXME: Should test exceptions! */
  f = fopen(fname, "w");
  fwrite(buf,1,bufsz,f);
  fclose(f);
}


int main(int argc, char *argv[]){
  smf_events *ev_original;
  misss_events *ev_misssified;
  smf_info *info;
  misssify_options *opt;
  ev_original = calloc(1, sizeof(smf_events));
  ev_misssified = calloc(1, sizeof(misss_events));
  info = calloc(1, sizeof(smf_info));
  opt = calloc(1, sizeof(misssify_options));
  
  misssify_options_parse(opt, argc, argv);
  dinput_size = file_read(opt->infname, dinput, MAX_DATA);
  deconstruct_from_midi(ev_original, info, dinput, opt);
  
  /*smf_events_printcontents(ev_original, stdout);*/
  
  /* 
     filter_events(?)??
  */
  construct_misss(ev_original, ev_misssified, info, opt);

  file_write(opt->outfname, ev_misssified->datapool, ev_misssified->ind);

  free(ev_original);
  free(ev_misssified);
  free(info);
  free(opt);
  return 0;
}
