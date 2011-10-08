/** Test the synth output via the SDL library */

#include <math.h>
#include "GL/gl.h"
#include "SDL/SDL.h"
#include "fcntl.h"
#include "sys/ioctl.h"
#include "unistd.h"

#include "synti2.h"

#define MY_SAMPLERATE 48000
#define AUDIOBUFSIZE  4096

/* stereo interleaved.. so SDL should have samples==AUDIOBUFSIZE/2 and
   bytes==AUDIOBUFSIZE / 4 for 16bit dynamic range (correct?)  */
float audiobuf[AUDIOBUFSIZE];

synti2_synth *st;
synti2_player *global_player;

static long frame = 0;

/* Test patch from the hack script: */
extern unsigned char hack_patch_sysex[];
extern int hack_patch_sysex_length;
extern unsigned char hacksong_data[];
extern unsigned int hacksong_length;


/**
 * Process sound with our own synthesis, then convert to SDL format.
 * TODO: Can we rely that we SDL really gave the requested format?
 */
static void sound_callback(void *udata, Uint8 *stream, int len)
{
  int i;
  float vol = 10000.0;
  
  /* Call our own synth engine and convert samples to native type (SDL) */
  /* Lengths are now hacked - will have stereo output from synti2.*/
  synti2_render(st, global_player,
		audiobuf, len/4); /* 4 = 2 bytes times 2 channels. */

  frame += len/4;
  for(i=0;i<len/2;i+=2){
    ((Sint16*)stream)[i+0] = (Sint16)(audiobuf[i/2]*vol); 
    ((Sint16*)stream)[i+1] = (Sint16)(audiobuf[i/2]*vol);
  }
}

/**
 * Some graphics
 */
static void render_scene_at_time(float time){
  int i;
  float segm;
  /* Some example graphics code */

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-1.33,1.33,-1,1,1.5,100);
  glMatrixMode(GL_MODELVIEW);
  glEnable(GL_DEPTH_TEST);
  glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

  glLoadIdentity();

  glTranslatef(sin(time),0, -30 + 5*sin(time));

  glRotatef(time*64,sin(time/7),sin(time),1);
  for(i=0; i<300; i++){
    glRotatef(12.0,1,0,1);
    glBegin(GL_TRIANGLES);
    glVertex3f(4+i%3,i*sin(time),150-i);
    glVertex3f(-1,sin(time*i),150-i);
    glVertex3f(3,i,150-i);
    glEnd();
  }

  segm = 2*3.1415926535 / 100;
  for(i=0; i<100; i++){

    glRotatef(360.0/100,0,0,1);

    glTranslatef(2*sin(time),0,0);
    
      glBegin(GL_TRIANGLES);
      glVertex3f(16+1,1,-5-10*sin(3*i*segm));
      glVertex3f(2+1,-1,-5-10*cos(3*i*segm));
      glVertex3f(6-1,1,-10);
      glEnd();


  }
  SDL_GL_SwapBuffers();
}

/** Try to wrap it... */
static void main2(int sdl_flags){
  SDL_Event event;
  SDL_AudioSpec aud;
  const SDL_VideoInfo * vid;
  float tnow;

  /* Checks of possible failures?*/
  st = synti2_create(MY_SAMPLERATE);
  synti2_do_receiveSysEx(st, hack_patch_sysex); /* hack.. */
  global_player = synti2_player_create(hacksong_data, MY_SAMPLERATE);

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

  /* Necessary?? Long names take up bytes!! (anyhow call before modeset) 
   * Actually, I think this should be quite unnecessary(?).
   */
  SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);

  /* It costs >35 bytes (compressed) to politely query the mode !! */
  vid = SDL_GetVideoInfo();  /* get desktop mode */
#if 0
  SDL_SetVideoMode(vid->current_w, vid->current_h,32,
		   sdl_flags|SDL_FULLSCREEN); /* use it*/
#endif
  SDL_SetVideoMode(640,400,32,sdl_flags);

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
      sound_callback(...FIXME...);
    }
    if (frame % 50 == 0){
      teh4k_render_at_time(tnow, snapshot);
    }
    frame++;
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
    tnow = (double) frame / MY_SAMPLERATE;
    render_scene_at_time(tnow);
    SDL_PollEvent(&event);
  } while (event.type!=SDL_KEYDOWN && tnow <70.0);

  SDL_CloseAudio();
  SDL_Quit();

#ifndef ULTRASMALL
  free(st);
  free(global_cont);
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
  asm (                                         \
       "xor %ebp,%ebp\n"                        \
       "and $0xfffffffffffffff0,%rsp"           \
       );
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
  int flags=SDL_OPENGL;
  /* I took this from MakeIt4k code; wonder why the second 'w' test?
   * (some platform compatibility? Why the first one not suffice?)
   */
  /*int flags=SDL_OPENGL|SDL_FULLSCREEN;*/
  if(argc>1)
    if(!strcmp(argv[1],"-w"))
      flags-=SDL_FULLSCREEN;
  if(argv[0][strlen(argv[0])-1]=='w')
    flags-=SDL_FULLSCREEN;

  main2(flags);
  return 0;
}
#endif
