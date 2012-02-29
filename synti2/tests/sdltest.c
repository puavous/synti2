/** Test the synth output via the SDL library */

#include <math.h>
#include "GL/gl.h"
#include "SDL/SDL.h"
#include "fcntl.h"
#include "sys/ioctl.h"
#include "unistd.h"

#include "synti2.h"

#define MY_SAMPLERATE 48000
#define AUDIOBUFSIZE  8192

/* stereo interleaved.. so SDL should have samples==AUDIOBUFSIZE/2 and
   bytes==AUDIOBUFSIZE / 4 for 16bit dynamic range (correct?)  */
float audiobuf[AUDIOBUFSIZE];

synti2_synth *st;

static long frame = 0;

/* Test patch from the hack script: */
extern unsigned char patch_sysex[];
extern unsigned char hacksong_data[];
extern unsigned int hacksong_length;


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

  frame += len/4;
  for(i=0;i<len/2;i+=2){
    ((Sint16*)stream)[i+0] = (Sint16)(audiobuf[i/2]*vol); 
    ((Sint16*)stream)[i+1] = (Sint16)(audiobuf[i/2]*vol);
  }
}


/** Try to wrap it... */
static void main2(int sdl_flags){
  SDL_Event event;
  SDL_AudioSpec aud;

  const SDL_VideoInfo * vid;
  float tnow;


  /* Checks of possible failures?*/
  st = synti2_create(MY_SAMPLERATE, patch_sysex, hacksong_data);

  /* Do some SDL init stuff.. */
  SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER);

  aud.freq     = MY_SAMPLERATE;
  aud.format   = AUDIO_S16SYS;
  aud.channels = 2;
  aud.samples  = AUDIOBUFSIZE/aud.channels;  /* "samples" means frames */
  aud.callback = sound_callback;
  aud.userdata = NULL;     /* Would be nice to use? &mySoundData; */
  /* btw, if userdata is not used in the callback, can delete reference*/


  /* NULL 2nd param makes SDL automatically convert btw formats. Nice! */
  SDL_OpenAudio(&aud, NULL);   /* Returns <0 upon failure. Check fits 4k?*/

  /* Necessary?? Long names take up bytes!! (anyhow call before
   * modeset) Actually, I think this should be quite unnecessary(?):
   *
   * SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
   */

  /* It costs 23-35 bytes (compressed) to politely query the mode !!
   * It is definitely worth the ease! Oooh, but it won't work with a
   * dual monitor setup! (tries to make fullscreen and crashes, afaik)
   */
  vid = SDL_GetVideoInfo();  /* get desktop mode */
#if 1
  SDL_SetVideoMode(vid->current_w, vid->current_h, 32,
		   sdl_flags|SDL_FULLSCREEN); /* use it*/
#else
  SDL_SetVideoMode(640,400,32,sdl_flags);
#endif

  /*SDL_SetVideoMode(1280,720,32,sdl_flags);*/
  /*SDL_SetVideoMode(1024,768,32,sdl_flags);*/

#ifdef TOFILEONLY
  /* Preliminary idea to just output to video and audio files. 
   * FIXME: Implement properly..
   */
  frame = 0;
  tnow = 0.0;
  do
  {
    tnow = (double) frame / MY_SAMPLERATE;
    if (frame % AUDIOBUFSIZE == 0){
      sound_callback(audbuffer, AUDIOBUFSIZE...FIXME...);
      write_to_snd(fout, audbuffer, AUDIOBUFSIZE)
    }
    if (frame % (MY_SAMPLERATE/50) == 0){  /* 50 fps */
      //teh4k_render_at_time(tnow, snapshot, AUDIOBUFSIZE);
      grab_screen_somehow_from_openGL_output(screenbuf);
      write_image_to_disk_for_later_encoding(..);
    }
    frame ++; //= AUDIOBUFSIZE;
  } while (tnow <70.0);
#endif

#ifndef ULTRASMALL
  SDL_WM_SetCaption("Testing soft synth SDL interface",0);
  SDL_ShowCursor(SDL_DISABLE);
#endif

  /* Start audio after inits are done.. */
  SDL_PauseAudio(0);

  do
  {
    //tnow = (double) frame / MY_SAMPLERATE;
    render_using_synti2(st); /* From 'Teh 4k 3000' */
    SDL_PollEvent(&event);
  } while (event.type!=SDL_KEYDOWN); // && tnow <70.0);

  /* Hmm... What happens if I don't close these? Will the world collapse!?*/
#ifndef ULTRASMALL
  SDL_CloseAudio();  /* Hmm.. Does SDL_Quit() do what this does? */
#endif

  SDL_Quit();  /* This must happen. Otherwise problems with exit!*/

#ifndef ULTRASMALL
  free(st);
#endif
}

/*
Hmm... what are they doing in __libc_start_main that is absolutely 
necessary... At the moment, my guess is that zeroing the 4 bits in
rsp is the most crucial thing. But the bp reset could also be nice.

=> 0x0000000000400de0 <+0>:	xor    %ebp,%ebp
   0x0000000000400de2 <+2>:	mov    %rdx,%r9
   0x0000000000400de5 <+5>:	pop    %rsi
   0x0000000000400de6 <+6>:	mov    %rsp,%rdx
   0x0000000000400de9 <+9>:	and    $0xfffffffffffffff0,%rsp
   0x0000000000400ded <+13>:	push   %rax
   0x0000000000400dee <+14>:	push   %rsp
   0x0000000000400def <+15>:	mov    $0x401c50,%r8
   0x0000000000400df6 <+22>:	mov    $0x401bc0,%rcx
   0x0000000000400dfd <+29>:	mov    $0x4015c1,%rdi
   0x0000000000400e04 <+36>:	callq  0x400c50 <__libc_start_main@plt>
 */

#ifdef ULTRASMALL
void _start()
/* Should check the architecure maybe.. the following assumes AMD64*/
{
#ifndef NO_I64
  asm (                                         \
       "xor %ebp,%ebp\n"                        \
       "and $0xfffffffffffffff0,%rsp"           \
       );
#endif

  main2(SDL_OPENGL);
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
  main2(SDL_OPENGL);
  return 0;
}
#endif
