/** (Planned; TODO: implement this.) A benchmark test that runs the
 * synthesizer 'off-line' as fast as it runs, and prints out the
 * execution time.  I'm going to use this for testing various
 * approaches to the inner loop implementation and stuff...
 * 
 * Also a good playground for making a Wav-writer.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include "synti2.h"
#include "synti2_guts.h"

/* How many seconds to run the synth. */
#define BENCHMARK_SECONDS 20

synti2_synth global_synth;

synti2_smp_t global_buffer[20000]; /* FIXME: limits? */

/* Test patch from the hack script: */
extern unsigned char patch_sysex[];

/* And a test song: */
extern unsigned char hacksong_data[];

int main(int argc, char *argv[])
{
  int iframe;
  unsigned int sr = 48000;
  int frames_at_once = 128;

  /* My own soft synth to be created. */
  synti2_init(&global_synth, sr, patch_sysex, hacksong_data);
  //  synti2_do_receiveSysEx(global_synth, hack_patch_sysex); /* hack.. */

  for(iframe=0; iframe < sr * BENCHMARK_SECONDS; iframe += frames_at_once){
    synti2_render(&global_synth, 
                  global_buffer, frames_at_once); 
  }
  return 0;
}
