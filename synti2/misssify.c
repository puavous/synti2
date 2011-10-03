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

/** Event layers. Linear pool of nodes. Data pointers can be data(?)*/
typedef struct events{
  evlist_node nodepool[MAX_NODES];
  int nextfree;

  /* One list for each possible two-byte combination (status byte and
   * first parameter)... We'll be doing a maximal filtering job later
   * on.. maybe?
   */
  evlist_node *head[0x10000];
} events;


#define MAX_STRINGLENGTH 200

/* Options for MIDI mutilation. */
typedef struct misssify_options{
  char infname[MAX_STRINGLENGTH+1];
  char outfname[MAX_STRINGLENGTH+1];
  int verbose;
  int options_valid;
} misssify_options;


#define MAX_DATA 0x1000000

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
  if (opt->verbose)
    puts ("verbose flag is set");
  
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



int main(int argc, char *argv[]){
  events *ev_original;
  events *ev_misssified;
  misssify_options opt;
  ev_original = calloc(1, sizeof(events));
  ev_misssified = calloc(1, sizeof(events));

  misssify_options_parse(&opt, argc, argv);
  dinput_size = file_read(opt.infname, dinput, MAX_DATA);
  /* 
     deconstruct_midi(events_original, dinput, dmidi);
     construct_misss(events_original, events_misssified, dmisss);
  */
  /*FIXME: Hack..*/ memcpy(dmisss, dinput, dmisss_size = dinput_size);
  file_write(opt.outfname, dmisss, dmisss_size);

  free(ev_original);
  free(ev_misssified);
  return 0;
}
