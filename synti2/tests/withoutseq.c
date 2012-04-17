/** Test the synth output via the SDL library, without the "bloat" of
 *  sequencer and song data. Generate note ons on-the-fly in the sound
 *  callback.
 */

#include "SDL/SDL.h"
#include "synti2.h"
#include "synti2_guts.h"

#include <stdlib.h>

#define MY_SAMPLERATE 48000
#define AUDIOBUFSIZE  2048

/* stereo interleaved.. so SDL should have samples==AUDIOBUFSIZE/2 and
   bytes==AUDIOBUFSIZE / 4 for 16bit dynamic range (correct?)  */
float audiobuf[AUDIOBUFSIZE];

synti2_synth st;

extern unsigned char patch_sysex[];

static void produce_hell(){
  static unsigned int note[] = {0 , 33, 55, 27};
  static unsigned int pat[] = {
    0x00000000,
    0x922292ae,
    0x08090809,
    0xbaaabaeb};
  static int tick = -1;
  //static int beat = 0;
  //static int neat = 0;
  static float hmm = 0.f;
  int i;
  int bits;

  tick++;
  //beat = tick >> 4;
  //tick &= 0x3f;

#ifdef EXTREME_NO_SEQUENCER

  
  if ((tick) % 8 == 0){
    hmm += .02f;
    st.patch[0].fpar[SYNTI2_F_LV1] = sin(hmm *.02f);
    st.patch[0].fpar[SYNTI2_F_LV2] = sin(hmm *.05f);
    st.patch[0].fpar[SYNTI2_F_LV3] = sin(hmm *.49f);
    //st.patch[0].fpar[SYNTI2_F_LV4] = .4f+sin(hmm * .01f);
    //synti2_do_noteon(&st, 0, 32+beat*3+beat, 100);
    synti2_do_noteon(&st, 0, 42+(pat[1]&13), 100);
    for(i=0;i<4;i++){
      bits = pat[i];
      if (bits >> 31) {
	synti2_do_noteon(&st, i, note[i], 100);
      }
      pat[i] = (bits >> 31) ? (bits << 1)+1 : (bits << 1);
    }
  }

  /*
  if ((tick) % 32 == 0){
    synti2_do_noteon(&st, 1, 27, 100);
  }

  if ((tick) % 64 == 32){
    synti2_do_noteon(&st, 2, 62, 100);
  }
  */

  /*
  if ((tick) % 32 == 0){
    neat += 1;
    neat &= 0x7f;
    synti2_do_noteon(&st, 3, neat, 100);
  }
  */
#endif
}

/**
 * Process sound with our own synthesis, then convert to SDL format.
 * TODO: Check if we can rely on SDL giving us the requested format?
 * (If I understood the doc, it says that SDL does any necessary
 * conversion internally)
 */
static void sound_callback(void *udata, Uint8 *stream, int len)
{
  int i;
  static const float vol = 30000.0f;
  
  produce_hell();
  /* Call our own synth engine and convert samples to native type (SDL) */
  /* Lengths are now hacked - will have stereo output from synti2.*/
  synti2_render(&st, 
		audiobuf, len/4); /* 4 = 2 bytes times 2 channels. */

  for(i=0;i<len/2;i+=2){
    ((Sint16*)stream)[i+0] = /*(Sint16)(audiobuf[i/2]*vol); */
    ((Sint16*)stream)[i+1] = (Sint16)(audiobuf[i]*vol);
  }
}

/** Try to wrap it... */
static void main2(){

#ifndef DRYRUN
  SDL_AudioSpec aud;

  synti2_init(&st, MY_SAMPLERATE, patch_sysex, NULL);

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

#else /*else do DRYRUN */
  /* This should effectively do the tiniest thing possible with synti2: */
  synti2_init(&st, MY_SAMPLERATE, patch_sysex, NULL);
  sound_callback(NULL, audiobuf, AUDIOBUFSIZE);

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
