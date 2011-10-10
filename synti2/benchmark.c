/** (Planned; TODO: implement this.) A benchmark test that runs the
 * synthesizer as fast as it runs, and prints out the execution time.
 * I'm going to use this for testing various approaches to the inner
 * loop implementation and stuff...
 * 
 * Also a good playground for making a Wav-writer.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include "synti2.h"

/* How many seconds to run the synth. */
#define BENCHMARK_SECONDS 10

synti2_conts *global_cont;
synti2_synth *global_synth;
synti2_player *global_player;

synti2_smp_t global_buffer[20000]; /* FIXME: limits? */

/* Test patch from the hack script: */
extern unsigned char hack_patch_sysex[];
extern int hack_patch_sysex_length;

/* And a test song: */
extern unsigned char hacksong_data[];
extern unsigned int hacksong_length;

int main(int argc, char *argv[])
{
  int iframe;
  unsigned int sr = 48000;
  int frames_at_once = 128;

  /* My own soft synth to be created. */
  global_synth = synti2_create(sr, hack_patch_sysex);
  if (global_synth == NULL){
    fprintf (stderr, "Couldn't allocate synti-kaksi \n");
    exit(1);
  }
  //  synti2_do_receiveSysEx(global_synth, hack_patch_sysex); /* hack.. */

  /*hack. FIXME: remove.*/
  global_player = synti2_player_create(hacksong_data, sr);
  if (global_player == NULL){
    fprintf(stderr, "No player could be made \n");
    exit(1);
  }

  for(iframe=0; iframe < sr * BENCHMARK_SECONDS; iframe += frames_at_once){
    synti2_render(global_synth, global_player,
                  global_buffer, frames_at_once); 
  }
  return 0;
}
