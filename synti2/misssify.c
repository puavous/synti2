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

/* -- linked list of timed events (easy to add, merge, and iterate
      linearly from beginning to end) -- */
/* Plan this here; move to a separate unit soon..*/

typedef struct evlist_node evlist_node;

struct evlist_node{
  unsigned int tick;
  void *data;
  evlist_node *next;
};

#define MAX_NODES 0x10000
#define MAX_DATA 0x1000000
#define MAX_STRINGLENGTH 200

/** Event layers. Linear pool of nodes. Data pointers can be data(?)*/
typedef struct events{
  evlist_node nodepool[MAX_NODES];
  unsigned char datapool[MAX_DATA];
  int next_free_node;
  int next_free_data;
  

  /* One list for each possible two-byte combination (status byte and
   * first parameter)... We'll be doing a maximal filtering job later
   * on.. maybe?
   */
  evlist_node *head[0x10000];
} events;

/* events_add */

/* Options for MIDI mutilation. */
typedef struct misssify_options{
  char infname[MAX_STRINGLENGTH+1];
  char outfname[MAX_STRINGLENGTH+1];
  int verbose;
  int options_valid;
} misssify_options;


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



/** Reads a MIDI variable length number. */
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

static
int
deconstruct_track(events *evs, 
                  unsigned char *dinput, 
                  misssify_options *opt)
{
  int chunk_size = 0;
  chunk_size = smf_read_int(dinput+4, 4);
  if (memcmp(dinput,"MTrk",4) != 0) {
    if (opt->verbose)
      fprintf(stderr, "Alien chunk type (\"%4s\"). Skipping\n", dinput);
    // info->ntracks_alien++;
  }
  if (opt->verbose)
    fprintf(stderr, "Chunk type (\"%4s\"). Length %d \n", dinput, chunk_size);
  dinput += 8; /* Move past type and size, to first event (which must exist) */
  
  return chunk_size;
}


static
void
deconstruct_from_midi(events *ev_original, 
                      unsigned char *dinput, 
                      misssify_options *opt)
{
  /* TODO: these to a struct(?): FIXME: Yes, put these to a struct:*/
  int chunk_size = 0;
  int ntracks_total = 0;
  int ntracks_read = 0;
  int ntracks_alien = 0;
  int smf_format = 0;
  int time_division = 0;
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
  smf_format = smf_read_int(dinput,2); dinput += 2;
  ntracks_total = smf_read_int(dinput,2); dinput += 2;
  time_division = smf_read_int(dinput,2); dinput += 2;
  if (opt->verbose){
    fprintf(stderr, 
            "SMF header: format %d ; ntracks %d ; time_division 0x%04x (=%d)\n",
            smf_format, ntracks_total, time_division, time_division);
    fprintf(stderr,
            "I'm not a MIDI validator, so I'm not checking file consistency!\n");
  }
  if (smf_format > 1) {
    fprintf(stderr,
            "Midi file is Type %d but only Type 0 and 1 are supported, sorry.\n");
    exit(1);
  }
  if (time_division & 0x8000) {
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
  for (itrack=0; itrack<ntracks_total; itrack++){
    if (opt->verbose){
      fprintf(stderr, "Processing track %d/%d (index=%d)\n", 
              itrack+1, ntracks_total, itrack);
    }
    chunk_size = deconstruct_track(ev_original, dinput, opt);
    dinput += 4 + 4 + chunk_size; /*<type(4b)><length(4b)><event(chunk_size)>+*/
  }
}


int main(int argc, char *argv[]){
  events *ev_original;
  events *ev_misssified;
  misssify_options opt;
  ev_original = calloc(1, sizeof(events));
  ev_misssified = calloc(1, sizeof(events));

  misssify_options_parse(&opt, argc, argv);
  dinput_size = file_read(opt.infname, dinput, MAX_DATA);
  deconstruct_from_midi(ev_original, dinput, &opt);
     
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
