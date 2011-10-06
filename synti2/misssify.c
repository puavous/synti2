/** MISSS - Midi-like interface for Synti2 Software Synthesizer.
 *
 * This program reads and analyzes a standard MIDI file (type 0 or
 * type 1), tries to mutilate it in various ways to reduce its size,
 * and outputs a file compatible with the super-simplistic sequencer
 * part of the Synti2 Software Synthesizer.
 *
 * FIXME: Now I just need to implement this.
 *
 * Ideas:
 *
 * - Trash all events that Synti2 doesn't handle.
 *
 * - Use simplistic note-off (not going to use note-off velocity)
 *
 * - Make exactly one "track" - call it "layer"! - per one MIDI status
 *   word, use the channel nibble of the status byte for something
 *   more useful (which is? Could be "chord size", i.e., number of
 *   instantaneous notes, no need for zero delta!), and write
 *   everything as "running status". Make a very simple layering
 *   algorithm to piece it together upon sequencer creation. Just "for
 *   layer in layers do song.addlayer(layer)" and then start
 *   playing. Will be very simple at that point.
 *
 * - Trash header information that Synti2 doesn't need.
 *
 * - Only one BPM value allowed throughout the song; no tempo map,
 *   sry. (No need to compute much during the playback).
 *
 * - Use lower parts-per-quarter setting, and quantize notes as
 *   needed. The variable-length delta idea is useful as it is.
 *
 * - Decimate velocities
 *
 * - Approximate controllers with linear ramps.
 *
 * - Turn percussion tracks to binary beat patterns.
 *
 * - Observe: Percussive instruments need no note-off.
 *
 */

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
 *  - 0x8 note off
 *        <delta>
 *  - 0x9 note on with variable velocity
 *        <delta><note#><velocity>
 *  - 0x1 note on with constant velocity (given as a parameter)
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
 *  - 0x5 pitch bend ramp from-to/during (or re-use controller ramp?
 *        DEFINITELY! because our values can be var-length which is 
 *        very natural for pitch bend MSB when needed..)
 *        <delta><bend1><delta2><bend2>
 *  - 0xf sysex
 *        <delta><sysex>
 *
 *  - 0x6 0x7 0xA 0xC 0xD 0xE reserved. 0x5 probably too.
 *    Maybe could use the 4th bit of type nibble for something else?
 *
 *  Type and part fit in one byte.
 */

#include<stdlib.h>
#include<stdio.h>
#include<getopt.h>
#include<string.h>

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
  evlist_node *next;
  /* TODO(?): int orig_track; // from which track */
};

/** Event layers. Linear pool of nodes. Data pointers can be data(?)*/
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



/* Options for MIDI mutilation. */
typedef struct misssify_options{
  char infname[MAX_STRINGLENGTH+1];
  char outfname[MAX_STRINGLENGTH+1];
  int verbose;
  int options_valid;
} misssify_options;



/* Information about the midi file and the process. */
typedef struct smf_info{
  int ntracks_total;
  int ntracks_read;
  int ntracks_alien;
  int smf_format;
  int time_division;

  float bpm;
  int timesig;
  int bpm_initialized;
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
        {"create",  required_argument, 0, 'c'},
        {"file",    required_argument, 0, 'f'},
        {0, 0, 0, 0} /* marks the end of options */
      };
    /* getopt_long stores the option index here. */
    int option_index = 0;
    
    c = getopt_long (argc, argv, "abc:d:f:",
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
        
      case 'f':
        printf ("option -f with value `%s'\n", optarg);
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
smf_events_rewind_iter(smf_events *evs, int extended_status){
  evs->iter[extended_status] = evs->head + extended_status;
}

static
void
smf_events_rewind_all_iters(smf_events *evs){
  int i;
  for (i=0;i<0x10000; i++){
    smf_events_rewind_iter(evs, i);
  }
}

/* smf_events_deepcpy_data */ /* actually memcpy does this.. */
/* smf_events_merge */

/* Merges a midi event to its appropriate location. Must store events
 * in chronological order after rewinding evs. Datalen must be given
 * as the midi spec says.
 */
static
void
smf_events_merge_new(smf_events *evs,
                     unsigned char *data,
                     int datalen,
                     unsigned int tick)
{
  int extended_status = evs->last_status * 0x100 + (*data);
  evlist_node *newnode;
  unsigned char *newdata;

  if (tick < (evs->iter[extended_status])->tick){
    fprintf(stderr, "Event ordering error. Skipping event!");
    return;
  }
  
  /* FIXME: intelligible error message */
  if (evs->next_free_node == MAX_NODES) exit(32);
  newnode = &(evs->nodepool[evs->next_free_node++]);

  /* if there is just 1 byte of data, it is already in extended_status */
  if (datalen > 1){
    if (evs->next_free_data == MAX_DATA) exit(31);
    newdata = &(evs->datapool[evs->next_free_data]);
    evs->next_free_data += (datalen-1);
    memcpy(newdata, data, datalen-1);
  } else {
    newdata = NULL;
  }
  newnode->tick = tick;
  newnode->data = newdata;

  /* Find insert location. */
  while((evs->iter[extended_status]->next != NULL)
        && (evs->iter[extended_status]->next->tick < tick)){
    evs->iter[extended_status]++;
  }

  /* Iterator now points to node after which the insertion is to be made. */
  newnode->next = evs->iter[extended_status]->next;
  evs->iter[extended_status]->next = newnode;
  evs->iter[extended_status] = newnode; /* don't change ordering of incoming data */
  evs->n_stored++;
}

static
int
smf_event_contents_from_midi(smf_events *evs, 
                             unsigned char *data, 
                             unsigned int tick)
{
  int vlval, vllen;
  switch(evs->last_status >> 4){
  case 0:
    fprintf(stderr, "Looks like a bug; sorry.\n"); /*assert status>0, actually.*/
    exit(1);
  case 8: case 9: case 0xa: case 0xb:
    smf_events_merge_new(evs,data,2,tick);
    return 2;
  case 0xc: case 0xd:
    smf_events_merge_new(evs,data,1,tick);
    return 1;
  case 0xe:
    smf_events_merge_new(evs,data,2,tick);
    return 2;
  }
  /* Now we know that the status is actually either SysEx or Meta*/
  if ((evs->last_status == 0xF0) || evs->last_status == 0xF7){
    /* SysEx */
    fprintf(stderr, "SysEx messages are not yet processed at all; sorry\n");
    vllen = smf_read_varlength(data, &vlval);
    return vllen + vlval;
    /* FIXME: Implement. */
  } else if (evs->last_status == 0xFF) {
    fprintf(stderr, "Meta events not yet processed at all; sorry\n");
    vllen = smf_read_varlength(data+1, &vlval);
    return 1 + vllen + vlval;
    /* FIXME: Implement. */
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
  smf_events *ev_misssified;
  misssify_options opt;
  smf_info info;
  ev_original = calloc(1, sizeof(smf_events));
  ev_misssified = calloc(1, sizeof(smf_events));

  misssify_options_parse(&opt, argc, argv);
  dinput_size = file_read(opt.infname, dinput, MAX_DATA);
  deconstruct_from_midi(ev_original, &info, dinput, &opt);
     
  /* 
     filter_events(?)??
     construct_misss(events_original, events_misssified, dmisss);
  */
  /*FIXME: Hack..*/ memcpy(dmisss, dinput, dmisss_size = dinput_size);
  file_write(opt.outfname, dmisss, dmisss_size);

  free(ev_original);
  free(ev_misssified);
  return 0;
}
