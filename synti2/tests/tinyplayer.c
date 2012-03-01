/** Test the synth output via the SDL library */

#include "SDL/SDL.h"
#include "synti2.h"

#include <stdlib.h>

#define MY_SAMPLERATE 48000
#define AUDIOBUFSIZE  8192

/* stereo interleaved.. so SDL should have samples==AUDIOBUFSIZE/2 and
   bytes==AUDIOBUFSIZE / 4 for 16bit dynamic range (correct?)  */
float audiobuf[AUDIOBUFSIZE];

synti2_synth *st;

/* Test patch from the hack script: */
extern unsigned char patch_sysex[];
extern unsigned char hacksong_data[];

/**
 * Process sound with our own synthesis, then convert to SDL format.
 * TODO: Check if we can rely on SDL giving us the requested format?
 * (If I understood the doc, it says that SDL does any necessary
 * conversion internally)
 */
static void sound_callback(void *udata, Uint8 *stream, int len)
{
  int i;
  float vol = 20000.0;
  
  /* Call our own synth engine and convert samples to native type (SDL) */
  /* Lengths are now hacked - will have stereo output from synti2.*/
  synti2_render(st, 
		audiobuf, len/4); /* 4 = 2 bytes times 2 channels. */

  for(i=0;i<len/2;i+=2){
    ((Sint16*)stream)[i+0] = (Sint16)(audiobuf[i/2]*vol); 
    ((Sint16*)stream)[i+1] = (Sint16)(audiobuf[i/2]*vol);
  }
}

/** Try to wrap it... */
static void main2(){

#ifndef DRYRUN
  SDL_AudioSpec aud;

  st = synti2_create(MY_SAMPLERATE, patch_sysex, hacksong_data);

  /* Do some SDL init stuff.. */
  SDL_Init(SDL_INIT_AUDIO);

  aud.freq     = MY_SAMPLERATE;
  aud.format   = AUDIO_S16SYS;
  aud.channels = 2;
  aud.samples  = AUDIOBUFSIZE/aud.channels;  /* "samples" means frames */
  aud.callback = sound_callback;
  aud.userdata = NULL;     /* Would be nice to use? &mySoundData; */

  SDL_OpenAudio(&aud, NULL);   /* Returns <0 upon failure. Check fits 4k?*/

  /* Start audio after inits are done.. */
  SDL_PauseAudio(0);

  for(;;) pause(1);

  /* Hmm... What happens if I don't close these? Will the world collapse!?*/
#ifndef ULTRASMALL
  SDL_CloseAudio();  /* Hmm.. Does SDL_Quit() do what this does? */
#endif

  SDL_Quit();  /* This must happen. Otherwise problems with exit!*/

#else /*end DRYRUN */
  /* This should effectively do the tiniest thing possible with synti2: */
  st = synti2_create(MY_SAMPLERATE, patch_sysex, hacksong_data);
  sound_callback(NULL, audiobuf, AUDIOBUFSIZE);

#endif


#ifndef ULTRASMALL
  free(st);
#endif
}


#ifdef ULTRASMALL
void
__attribute__ ((externally_visible)) 
_start()
{
#ifndef NO_I64
  /* AMD64 requires stack alignment */
  asm (                                         \
       "xor %rbp,%rbp\n"                        \
       "and $0xfffffffffffffff0,%rsp"           \
       );
#endif

#ifndef NOTHING
  main2();
#endif
  /* Inline assembler for exiting without need of stdlib */
  asm (                                         \
       "movl $1,%eax\n"                         \
       "xor %ebx,%ebx\n"                        \
       "int $128\n"                             \
                                                );
}
#else
  int main(int argc, char *argv[])
{
  main2();
  return 0;
}
#endif
