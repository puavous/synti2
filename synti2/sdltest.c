/** Test the synth output via the SDL library */

#include <math.h>
#include "GL/gl.h"
#include "SDL/SDL.h"
#include "fcntl.h"
#include "sys/ioctl.h"
#include "unistd.h"

#include "synti2.h"

#define AUDIOBUFSIZE  2048
/* stereo interleaved.. so SDL should have samples==AUDIOBUFSIZE/2 and
   bytes==AUDIOBUFSIZE / 4 (correct?)  */
float audiobuf[AUDIOBUFSIZE];

synti2_conts *global_cont;
synti2_synth *st;

static long frame = 0;

/**
 * Process sound with our own synthesis, then convert to SDL format.
 * TODO: Can we rely that we SDL really gave the requested format?
 */
void sound_callback(void *udata, Uint8 *stream, int len)
{
  int i;
  float vol = 10000.0;
  
  unsigned char hackbuf[80];
  
  /* Make some test events.. */
  synti2_conts_reset(global_cont);

  if ((rand() * (1.0/RAND_MAX)) < 0.053){
    hackbuf[0] = 0x90;
    hackbuf[1] = 0x10 + (rand() * (1.0/RAND_MAX))*0x20;
    hackbuf[2] = frame / 8000 % 0x80;
  } else if ((rand() * (1.0/RAND_MAX)) < 0.055) {
    hackbuf[0] = 0x80;   hackbuf[1] = 0x40;   hackbuf[2] = 0x7f;
  }

  synti2_conts_store(global_cont, 10, hackbuf, 3);

  synti2_conts_start(global_cont);

  /* Call our own synth engine and convert samples to native type (jack) */
  synti2_render(st, global_cont,
		audiobuf, len/2); /* 4 = 2 bytes times 2 channels. */

  frame += len/4;
  for(i=0;i<len/2;i+=2){
    ((Sint16*)stream)[i+0] = (Sint16)(audiobuf[i+0]*vol); 
    ((Sint16*)stream)[i+1] = (Sint16)(audiobuf[i+1]*vol);
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


#ifdef ULTRASMALL
void _start()
#else
  int main(int argc, char *argv[])
#endif
{
  SDL_Event event;
  SDL_AudioSpec w;

  /* I took this from MakeIt4k code; wonder why the second 'w' test?
   * (some platform compatibility? Why the first one not suffice?)
   */
  int flags=SDL_OPENGL|SDL_FULLSCREEN;
  float tnow;

#ifndef ULTRASMALL
  if(argc>1)
    if(!strcmp(argv[1],"-w"))
      flags-=SDL_FULLSCREEN;
  if(argv[0][strlen(argv[0])-1]=='w')
    flags-=SDL_FULLSCREEN;
#endif

  /* Checks of possible failures?*/
  st = synti2_create(48000);
  global_cont = synti2_conts_create();

  /* Do some SDL init stuff.. */
  SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER);
  SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);

  w.freq=48000;
  w.format=AUDIO_S16SYS;
  w.channels=2;
  w.samples=AUDIOBUFSIZE/2;  /* in SDL "samples" means stereo frames(?) */

  w.callback=sound_callback;
  w.userdata=NULL;           /* Would be nice to use? &mySoundData; */

  SDL_OpenAudio(&w,NULL);    /* Returns <-1 upon failure. Check fits 4k?*/
                             /* Should we also check the second struct parm?*/

  /* How do I query the qurrent display mode and use that? */
  /*SDL_SetVideoMode(1280,800,32,flags+SDL_FULLSCREEN);*/
  /*SDL_SetVideoMode(1280,720,32,flags);*/
  SDL_SetVideoMode(1024,768,32,flags);
  /*SDL_SetVideoMode(800,600,32,flags);*/

#ifdef TOFILEONLY
  /* Preliminary idea to just output to video and audio files. */
  frame = 0;
  tnow = 0.0;
  do
  {
    tnow = frame / 48000.0;
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
    tnow = frame / 48000.0;
    render_scene_at_time(tnow);
    SDL_PollEvent(&event);
  } while (event.type!=SDL_KEYDOWN && tnow <70.0);

  SDL_CloseAudio();
  SDL_Quit();

#ifndef ULTRASMALL
  free(st);
  free(global_cont);
#endif

#ifdef ULTRASMALL
  /* Inline assembler for exiting without need of stdlib */
  asm (                                         \
       "movl $1,%eax\n"                         \
       "xor %ebx,%ebx\n"                        \
       "int $128\n"                             \
                                                );
#else
  return 0;
#endif
}
