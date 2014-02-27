/** A main program that opens graphics and sound output via the SDL
 *  library, and plays back a song using Synti2. Song, sounds, and
 *  graphics can be changed by including different files in the
 *  compilation.
 *
 *  Developed and tested in a Linux setting, but I suppose this should
 *  work on Windows.
 */

#include <math.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "GL/gl.h"
#include "GL/glx.h"
#include "SDL/SDL.h"

#include "synti2.h"
#include "synti2_guts.h"

#define MY_SAMPLERATE 48000
#define AUDIOBUFSIZE  8192

#include "glfuncs.c"

/* The shaders */
#include "shaders.c"

/* These datas are created by the tool programs: */
/* I'm dirty enough to just include them: */
#include "patchdata.c"
#include "songdata.c"

#include "synti2.c"

typedef void *func_t(void);
func_t *myglfunc[NUMFUNCTIONS];

/* stereo interleaved.. so SDL should have samples==AUDIOBUFSIZE/2 and
   bytes==AUDIOBUFSIZE / 4 for 16bit dynamic range (correct?)  */
float audiobuf[AUDIOBUFSIZE];

synti2_synth global_synth;
//float synthtime;

/* Global variables for window size (to be passed on to the shader in render.c) */
/* Needed nowadays for proper coordinate computations: */
static int window_h = 600;
static float ar;
/* For autodetecting reso: */
SDL_VideoInfo myVideoInfo;
SDL_VideoInfo * vid;


/* gl shader stuff... global. Hmm. cost/gain of putting into a struct? */
GLuint vsh,fsh,pid;

/* The rendering functions: */
#include "render.c"

/**
 * Process sound with our own synthesis, then convert to SDL format.
 * TODO: Check if we can rely on SDL giving us the requested format?
 * (If I understood the doc, it says that SDL does any necessary
 * conversion internally)
 */
static 
void 
sound_callback(void *udata, Uint8 *stream, int len)
{
  int i;
  float vol = 32767.0f; /* Synti2 waveshaper squashes output to [-1,1] */
  
  /* Call the synth engine and convert samples to native type (SDL) */
  /* Lengths are now hacked - will have stereo output from synti2.*/
  synti2_render(&global_synth, 
                audiobuf, len/4); /* 4 = 2 bytes times 2 channels. */
  
  //  frame += len/4;
  for(i=0;i<len/2;i+=2){
#ifdef FEAT_STEREO
    ((Sint16*)stream)[i+0] = (Sint16)(audiobuf[i+0]*vol); 
    ((Sint16*)stream)[i+1] = (Sint16)(audiobuf[i+1]*vol);
#else
    ((Sint16*)stream)[i+1] = (((Sint16*)stream)[i+0]
      = (Sint16)(audiobuf[i+0]*vol)); 
#endif
  }
}


/* For Shader debugging, when ULTRASMALL is not used. */
static void printShaderInfoLog(GLuint obj)
{
  int infologLength = 0;
  int charsWritten  = 0;
  char *infoLog;
  
  glGetShaderiv(obj, GL_INFO_LOG_LENGTH,&infologLength);
  
  if (infologLength > 0)
    {
      infoLog = (char *)malloc(infologLength);
      glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
      printf("%s\n",infoLog);
      free(infoLog);
    }
}

/* For Shader debugging, when ULTRASMALL is not used. */
static void printProgramInfoLog(GLuint obj)
{
  int infologLength = 0;
  int charsWritten  = 0;
  char *infoLog;
  
  glGetProgramiv(obj, GL_INFO_LOG_LENGTH,&infologLength);
  
  if (infologLength > 0)
    {
      infoLog = (char *)malloc(infologLength);
      glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
      printf("%s\n",infoLog);
      free(infoLog);
    }
}



static void init_or_die(){
  SDL_AudioSpec aud;
  int i;
  
  /* Do some SDL init stuff.. */
  SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER);
  
  for(i=0; i<NUMFUNCTIONS;i++)
    {
      myglfunc[i] = glXGetProcAddress( (const unsigned char *)strs[i] );
      
#ifdef NEED_DEBUG
      printf("Func %d at: %lx  (\"%s\")\n",i, myglfunc[i],strs[i]);
      if( !myglfunc[i] ){
        exit(1);
      }
#endif
    }
  
  /* It costs 23-35 bytes (compressed) to politely query the display
   * mode. But it is definitely worth the ease! Oooh, but it won't
   * work with a dual monitor setup! (tries to make fullscreen and
   * crashes, afaik) So, probably we'll have to make an option for
   * this... or just compile two versions, One for fullscreen and one
   * for windowed mode.
   */
  
#ifndef NO_FULLSCREEN
  /* Operations for fullscreen show: */
  /* Hacked this for today's show: */
  //#define SW 1280
  //#define SH 720
  vid = &myVideoInfo;
  vid->current_w = 1280;
  vid->current_h = 720;
  SDL_SetVideoMode(vid->current_w, vid->current_h, 32,
                   SDL_OPENGL|SDL_FULLSCREEN);
  window_h = vid->current_h;
  ar = (float)vid->current_w/vid->current_h;

#if 0
  /* "Usual operation, SDL autodetect fullscreen */
  vid = SDL_GetVideoInfo();  /* get desktop mode */
  SDL_SetVideoMode(vid->current_w, vid->current_h, 32,
                   SDL_OPENGL|SDL_FULLSCREEN);
  window_h=vid->current_h;
  ar = (float)vid->current_w/vid->current_h;
#endif

#else
  /* Operations for development mode (not fullscreen) */
  /* FIXME: Make video mode changeable from compilation? ifdef H800 ..*/
  ar = 1280.f/720.f; //16.f/9.f;
  window_h = 90;
  SDL_SetVideoMode(ar*window_h,window_h,32,SDL_OPENGL);
#endif

#ifndef ULTRASMALL
  SDL_WM_SetCaption("Soft synth SDL interface",0);
#endif
  /* For Assembly 2013 they need the cursor hidden always: */
  SDL_ShowCursor(SDL_DISABLE);
  
  /* They say that OpenAudio needs to be called after video init. */
  aud.freq     = MY_SAMPLERATE;
  aud.format   = AUDIO_S16SYS;
  aud.channels = 2;
  aud.samples  = AUDIOBUFSIZE/2;  /* "samples" means frames */
  aud.callback = sound_callback;
  /*aud.userdata = NULL;*/
  /* My data is global, so userdata reference is not used */
  
  /* NULL 2nd param makes SDL automatically convert btw formats. Nice! */
#ifndef ULTRASMALL
  if (SDL_OpenAudio(&aud, NULL) < 0) {
    printf("SDL_OpenAudio failed: %s\n", SDL_GetError());
    exit(2);
  };
#else
  SDL_OpenAudio(&aud, NULL);  /* Would return <0 upon failure.*/
#endif

  synti2_init(&global_synth, MY_SAMPLERATE, patch_sysex, hacksong_data);
    
  /* Ok.. These need to be done after SDL is initialized: */
  pid = oglCreateProgram();
  vsh = oglCreateShader(GL_VERTEX_SHADER);
  fsh = oglCreateShader(GL_FRAGMENT_SHADER);
  oglShaderSource(vsh,1,&vs,0);
  oglShaderSource(fsh,1,&fs,0);
  oglCompileShader(vsh);
  oglCompileShader(fsh);
  oglAttachShader(pid,vsh);
  oglAttachShader(pid,fsh);
  oglLinkProgram(pid);

#ifdef NEED_DEBUG
  printShaderInfoLog(vsh);
  printShaderInfoLog(fsh);
  printProgramInfoLog(pid);
#endif
  
}

/** Try to wrap it... */
static void main2(){
  SDL_Event event;
    
  init_or_die();
  
  SDL_PauseAudio(0); /* Start audio after inits are done.. */
  
  do {
    render_w_shaders(&global_synth,ar);

    //time = (float)(global_synth.framecount) / global_synth.sr;

    SDL_GL_SwapBuffers();
    SDL_PollEvent(&event);
  } while ((event.type!=SDL_KEYDOWN)); // && (synthtime <78.0f));
  
#ifndef ULTRASMALL
  SDL_CloseAudio();  /* This evil? Call to SDL_Quit() sufficient? */
#endif
  
  SDL_Quit();  /* This must happen. Otherwise problems with exit! */
}


#ifdef ULTRASMALL
void
__attribute__ ((externally_visible)) 
_start()
{
#ifndef NO_I64
  /* x64-64 requires stack alignment */
  asm (                                         \
       "xor %rbp,%rbp\n"                        \
       "and $0xfffffffffffffff0,%rsp"           \
                                                );
#endif
  
  main2();
  
  /* Inline assembler for exiting without need of stdlib */
  asm (                                         \
       "movl $1,%eax\n"                         \
       "xor %ebx,%ebx\n"                        \
       "int $128\n"                                 \
                                                );
}
#else
int main(int argc, char *argv[]){
  main2();
  return 0;
}
#endif
