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
 */

#include<stdlib.h>
#include<stdio.h>
#include<getopt.h>

/* -- an insert-only binary tree -- */
/* Plan this here; move to a separate unit soon..*/

typedef struct instree_node instree_node;

struct instree_node{
  unsigned int key;
  void *value;
  instree_node *left;
  instree_node *right;
};

#define MAX_NODES 0x10000

/* One for each possible two-byte combination... */
instree_node *trees[0x10000];

/* Linear pool of nodes. */
typedef struct instree{
  instree_node nodepool[MAX_NODES];
  int nextfree;
} instree;

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
unsigned char dmidi[MAX_DATA];
unsigned char dmisss[MAX_DATA];


/** Interprets command line.
 *
 * TODO: Implement this; so far only example code from the GNU manual.
 */
static
void
misssify_options_parse(misssify_options *opt, int argc, char *argv[]){
  int c;
  static int local_verbose; /* must know address on compile time. */
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
  
  /* Print any remaining command line arguments (not options). */
  if (optind < argc) {
    printf ("non-option ARGV-elements: ");
    while (optind < argc)
      printf ("%s ", argv[optind++]);
    putchar ('\n');
  }
}


int main(int argc, char *argv[]){
  instree *events_original;
  instree *events_misssified;
  misssify_options opt;
  events_original = calloc(1, sizeof(instree));
  events_misssified = calloc(1, sizeof(instree));

  misssify_options_parse(&opt, argc, argv);
  // file_read(opt.infname, dinput);
  // completemidi(events_original, dinput, dmidi);
  // do_misssify(events_original, events_misssified, dmisss);
  // file_writemisss(opt.outfname, events_misssified);

  free(events_original);
  free(events_misssified);
  return 0;
}
