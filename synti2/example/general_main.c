/** A main program that can be used for different purposes using
 *  macro switches. 
 *
 *  This crudely includes source files for rendering video and audio,
 *  so after preprocessing there is only one compilation unit.
 *
 *  Developed and tested in a Linux setting, but I suppose this should
 *  work on Windows with a couple of ifdefs.
 *
 * Currently supported switches (-DSWITCH for compiler):
 *
 *   ULTRASMALL              Produce smallest, unsafe code (for 4k intros)
 *
 * Choose one of:
 *
 *   SYNTH_PLAYBACK_SDL      Use SDL for audio (playback)
 *
 *   SYNTH_COMPOSE_JACK      Use jack for midi input and audio output
 *
 *   DUMP_FRAMES_AND_SNDFILE Offline dump of video frames and audio
 *   
 * Adjustables:
 *
 *   FULLSCREEN              Open video in fullscreen, changing
 *                           mode (otherwise creates a window).
 *
 *   SCREEN_WIDTH            Width of window/screen in pix
 *
 *   SCREEN_HEIGHT           Height of window/screen in pix
 *
 *   SCREEN_AUTODETECT       Width and height from desktop size
 *
 *   SAMPLE_RATE             Sample rate
 *
 *   AUDIO_BUFFER_SIZE       Audio buffer size
 *
 *
 * After using DUMP_FRAMES_AND_SNDFILE, these steps seem to yield a
 * working mp4 video.
 *
 *    for f in *.ppm; do echo $f; convert $f -quality 90 `basename $f .ppm`.jpg; done
 *    ffmpeg -r 50 -i frame_%05d.jpg -i audio.wav -strict -2 output.mp4
 *
 * # This hack was necessary earlier, for unknown reasons:
 *    for f in frame_000{00..19}.ppm; do cp frame_00020.ppm $f; done
 */

#include <math.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "GL/glew.h"
#include "GL/gl.h"
#include "GL/glx.h"
#include "SDL/SDL.h"

#include "synti2.h"
#include "synti2_guts.h"

#ifdef SYNTH_COMPOSE_JACK
#ifndef WIN32
#include <unistd.h>
#endif
#include <signal.h>
#include <jack/jack.h>
#include <jack/midiport.h>
typedef jack_default_audio_sample_t sample_t;
#endif

#ifdef DUMP_FRAMES_AND_SNDFILE
#include "sndfile.h"
#endif

#ifndef SAMPLE_RATE
#define SAMPLE_RATE 48000
#endif
#ifndef AUDIO_BUFFER_SIZE
#define AUDIO_BUFFER_SIZE 8192
#endif
#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 108
#endif
#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH (SCREEN_HEIGHT * (16./9))
#endif

/* For autodetecting reso: */
SDL_VideoInfo myVideoInfo;
SDL_VideoInfo * vid;

#ifdef SYNTH_COMPOSE_JACK
jack_client_t *client;
jack_port_t *output_portL;
jack_port_t *output_portR;
jack_port_t *inmidi_port;
unsigned long sr;
char * client_name = "jacksynti2";
synti2_smp_t global_buffer[20000]; /* FIXME: limits? */
#endif

float synthtime;

/* I'm dirty enough to just include everything in the same compilation
 * unit. Then I don't have to think about linking and special
 * link-time optimizations.
 */

/* These datas are created by the tool programs: */
#include "patchdata.c"
#include "songdata.c"

/* opengl functions bypassing normal linkage */
#include "glfuncs.c"

/* The shaders */
#include "shaders.c"

/* The synth engine. */
#include "synti2.c"

/* gl and shader stuff... global. Hmm. cost/gain of putting into a struct? */
typedef void *func_t(void);
func_t *myglfunc[NUMFUNCTIONS];
GLuint vsh,fsh,pid;

/* Global variables for window size (to be passed on to the shader in render.c) */
/* Needed nowadays for proper coordinate computations: */
static int window_h = SCREEN_HEIGHT;
static float ar;

/* The rendering functions: */
#include "render.c"


/* stereo interleaved.. so SDL should have samples==AUDIOBUFSIZE/2 and
   bytes==AUDIOBUFSIZE / 4 for 16bit dynamic range (correct?)  */
float audiobuf[AUDIO_BUFFER_SIZE];

synti2_synth global_synth;

#ifdef SYNTH_COMPOSE_JACK
static void signal_handler(int sig)
{
  jack_client_close(client);
  fprintf(stderr, "signal received, exiting ...\n");
  exit(0);
}

static int
sound_callback_jack (jack_nframes_t nframes, void *arg)
{
  int i;
  sample_t *bufferL = (sample_t*)jack_port_get_buffer(output_portL, nframes);
  sample_t *bufferR = (sample_t*)jack_port_get_buffer(output_portR, nframes);

  synti2_read_jack_midi(&global_synth, inmidi_port, nframes);
  synti2_render(&global_synth, global_buffer, nframes); 

  for (i=0;i<nframes;i++){
    bufferL[i] = global_buffer[2*i];
#ifndef FEAT_STEREO
    bufferR[i] = bufferL[i];
#else
    bufferR[i] = global_buffer[2*i+1];
#endif
  }
  return 0;
}
#endif


/**
 * Process sound with our own synthesis, then convert to SDL format.
 * TODO: Check if we can rely on SDL giving us the requested format?
 * (If I understood the doc, it says that SDL does any necessary
 * conversion internally)
 */
static 
void 
sound_callback_sdl(void *udata, Uint8 *stream, int len)
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

#ifdef DUMP_FRAMES_AND_SNDFILE
static void write_image(int n, int w, int h, unsigned char *data){
  char fname[80];
  int i;
  FILE *ff;
  sprintf(fname, "frame_%05d.ppm", n);
  ff = fopen(fname, "w");
  fprintf(ff, "P6\n");
  fprintf(ff, "%d %d %d\n",w,h,255);
  /* Lines in reverse order: */
  for(i=h-1;i>=0;i--){
    fwrite(&data[(3*i*w)],3,w,ff);
  }
  fclose(ff);
}

static void grab_frame(){
  static int n=0;
  unsigned char *data;
  data = malloc(3*(int)(ar*window_h*window_h));
  /*unsigned char data[3*(int)(ar*window_h*window_h)];*/
  n++;
  glReadPixels(0,0,ar*window_h,window_h,GL_RGB,GL_UNSIGNED_BYTE,data);
  write_image(n, ar*window_h, window_h, data);
  free(data);
}

#endif



#ifdef NEED_DEBUG
/* For Shader debugging, when NEED_DEBUG is set. */
static void printShaderInfoLog(GLuint obj)
{
  GLint infologLength = 0;
  int charsWritten  = 0;
  char *infoLog;

  /* 
  PFNGLGETSHADERIVPROC oglGetShaderiv;
  PFNGLGETSHADERINFOLOGPROC oglGetShaderInfoLog;

  oglGetShaderiv = glXGetProcAddress("glGetShaderiv");
  oglGetShaderInfoLog = glXGetProcAddress("glGetShaderInfoLog");
   */

  printf("printShaderInfoLog():\n"); fflush(stdout);
  glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
  
  if (infologLength > 0)
    {
      infoLog = (char *)malloc(infologLength);
      glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
      printf("%s\n",infoLog);
      free(infoLog);
    }
}

/* For Shader debugging, when NEED_DEBUG is set. */
static void printProgramInfoLog(GLuint obj)
{
  int infologLength = 0;
  int charsWritten  = 0;
  char *infoLog;

  /*
  PFNGLGETPROGRAMIVPROC oglGetProgramiv;
  PFNGLGETPROGRAMINFOLOGPROC oglGetProgramInfoLog;

  oglGetProgramiv = glXGetProcAddress("glGetProgramiv");
  oglGetProgramInfoLog = glXGetProcAddress("glGetProgramInfoLog");
  */

  printf("printProgramInfoLog():\n"); fflush(stdout);
  glGetProgramiv(obj, GL_INFO_LOG_LENGTH,&infologLength);
  
  if (infologLength > 0)
    {
      infoLog = (char *)malloc(infologLength);
      glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
      printf("%s\n",infoLog);
      free(infoLog);
    }
}
#endif


#ifdef SYNTH_COMPOSE_JACK
/** Initialize Jack realtime audio and midi; exit if problems
 *  occur. 
 */
static void init_or_die_jack_audio(){
  jack_status_t status;
  /* Initial Jack setup. Open (=create?) a client. */
  if ((client = jack_client_open (client_name, 
                                  JackNoStartServer, 
                                  &status)) == 0) {
    fprintf (stderr, "jack server not running?\n");
    exit(1);
  }

  /* Set up process callback */
  jack_set_process_callback (client, sound_callback_jack, 0);
  
  output_portL = jack_port_register (client, 
                                     "bportL", 
                                     JACK_DEFAULT_AUDIO_TYPE, 
                                     JackPortIsOutput, 
                                     0);

  output_portR = jack_port_register (client, 
                                     "bportR", 
                                     JACK_DEFAULT_AUDIO_TYPE, 
                                     JackPortIsOutput, 
                                     0);

  inmidi_port = jack_port_register (client, 
                                    "iportti", 
                                    JACK_DEFAULT_MIDI_TYPE, 
                                    JackPortIsInput, 
                                    0);

  sr = jack_get_sample_rate (client);

  /* My own soft synth to be created. */
#ifdef DO_INCLUDE_PATCH_AND_SONG
  synti2_init(&global_synth, sr, patch_sysex, hacksong_data);
#else
  synti2_init(&global_synth, sr, NULL, NULL);
#endif

  /* Now we activate our new client. */
  if (jack_activate (client)) {
    fprintf (stderr, "cannot activate client\n");
    exit(1);
  }
  
  /* install a signal handler to properly quit jack client */
#ifdef WIN32
  signal(SIGINT, signal_handler);
  signal(SIGABRT, signal_handler);
  signal(SIGTERM, signal_handler);
#else
  signal(SIGQUIT, signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGHUP, signal_handler);
  signal(SIGINT, signal_handler);
#endif

}
#endif

#ifdef SYNTH_PLAYBACK_SDL
static void init_or_die_sdl_audio(SDL_AudioSpec *aud){
  aud->freq     = SAMPLE_RATE;
  aud->format   = AUDIO_S16SYS;
  aud->channels = 2;
  aud->samples  = AUDIO_BUFFER_SIZE/2;  /* "samples" means frames */
  aud->callback = sound_callback_sdl;
  /* aud->userdata = NULL; */
  /* My data is global, so userdata reference is not used */
}
#endif

/** Initialize SDL video and audio, or exit if problems occur. */
static void init_or_die_sdl(){
#if SYNTH_PLAYBACK_SDL
  SDL_AudioSpec aud;
#endif
  int i;
  
  /* Do some SDL init stuff.. */
#if SYNTH_PLAYBACK_SDL
  SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER);
#elif SYNTH_COMPOSE_JACK || DUMP_FRAMES_AND_SNDFILE
  SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER);
#else
#error Where should I output sound??
#endif

  
  for(i=0; i<NUMFUNCTIONS;i++)
    {
      myglfunc[i] = (func_t*) glXGetProcAddress( (const unsigned char *)strs[i] );
      
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
   * crashes, afaik) So, leave it as optional compile.
   */
  
#ifdef SCREEN_AUTODETECT
  /* "Usual operation", SDL autodetect fullscreen */
  vid = SDL_GetVideoInfo(); /* from desktop settings */
#else
  /* Force a video mode. */
  vid = &myVideoInfo;
  vid->current_w = SCREEN_WIDTH;
  vid->current_h = SCREEN_HEIGHT;
#endif

#ifdef FULLSCREEN
  SDL_SetVideoMode(vid->current_w, vid->current_h, 32,
                   SDL_OPENGL|SDL_FULLSCREEN);
#else
  SDL_SetVideoMode(vid->current_w, vid->current_h, 32,
                   SDL_OPENGL);
#endif

  window_h = vid->current_h;
  ar = (float)vid->current_w / vid->current_h;

  state[1] = window_h*ar; /* globals */
  state[2] = window_h;    /* globals */

#ifndef ULTRASMALL
  SDL_WM_SetCaption("Soft synth SDL interface",0);
#endif
  /* For Assembly 2013 they need the cursor hidden always: */
  SDL_ShowCursor(SDL_DISABLE);
  
  /* They say that OpenAudio needs to be called after video init. */
#ifdef SYNTH_PLAYBACK_SDL
  /* NULL 2nd param makes SDL automatically convert btw formats. Nice! */
  init_or_die_sdl_audio(&aud);
#ifndef ULTRASMALL
  if (SDL_OpenAudio(&aud, NULL) < 0) {
    printf("SDL_OpenAudio failed: %s\n", SDL_GetError());
    exit(2);
  };
#else
  SDL_OpenAudio(&aud, NULL);  /* Would return <0 upon failure.*/
#endif
#endif

#if SYNTH_PLAYBACK_SDL || DUMP_FRAMES_AND_SNDFILE
  synti2_init(&global_synth, SAMPLE_RATE, patch_sysex, hacksong_data);
#endif
    
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
  GLenum gres = glewInit();
  if (gres != GLEW_OK){
    fprintf(stderr, "Error in GLEW init: %s\n", glewGetErrorString(gres));
    exit(1);
  }
  printShaderInfoLog(vsh);
  printShaderInfoLog(fsh);
  printProgramInfoLog(pid);
#endif
  
}

#ifdef DUMP_FRAMES_AND_SNDFILE
int fps;
size_t spf;
SF_INFO sfi;
SNDFILE *sf;

static void
init_or_die_libsndfile(){
  fps = 50;
  spf = SAMPLE_RATE / fps;
  sfi.samplerate = SAMPLE_RATE;
  sfi.channels = 2;
  sfi.format = SF_FORMAT_WAV + SF_FORMAT_FLOAT;
  /*sfi.frames = ;  sfi.sections = ;  sfi.seekable = ;*/
  sf = sf_open("audio.wav",SFM_WRITE,&sfi);
}

static void
dump_audio_for_one_frame(){
  size_t s;
  synti2_render(&global_synth, 
                audiobuf, spf);
  /* Mono dup. */
  for (s=0;s<spf*2;s+=2){
    audiobuf[s+1]=audiobuf[s];
  };
  sf_write_float(sf, audiobuf, 2*spf);
}
#endif


/** Try to wrap it... */
static void main2(){
  SDL_Event event;
    
  init_or_die_sdl();

#ifdef SYNTH_COMPOSE_JACK
  init_or_die_jack_audio();
#endif
  
#ifdef SYNTH_PLAYBACK_SDL
  SDL_PauseAudio(0); /* Start audio after inits are done.. */
#endif

#ifdef DUMP_FRAMES_AND_SNDFILE
  init_or_die_libsndfile();
#endif
  
  do {
#ifdef DUMP_FRAMES_AND_SNDFILE
    dump_audio_for_one_frame();
#endif

    synthtime = (float)global_synth.framecount / global_synth.sr;
    state[0] = synthtime;
    render_w_shaders(&global_synth);
    SDL_GL_SwapBuffers();

#ifdef DUMP_FRAMES_AND_SNDFILE
    grab_frame();
#endif

    SDL_PollEvent(&event);

#ifdef SYNTH_PLAYBACK_SDL
  } while ((event.type!=SDL_KEYDOWN)); // && (synthtime <78.0f));
#elif SYNTH_COMPOSE_JACK
  } while ((event.type!=SDL_QUIT));
#elif DUMP_FRAMES_AND_SNDFILE
  } while ((event.type!=SDL_KEYDOWN) && (synthtime < PLAYBACK_DURATION));
#else
#error End condition not determined by sound output
#endif
  
#ifdef SYNTH_PLAYBACK_SDL
#ifndef ULTRASMALL
  SDL_CloseAudio();  /* This evil? Call to SDL_Quit() sufficient? */
#endif
#elif SYNTH_COMPOSE_JACK
  jack_client_close(client);
#elif DUMP_AUDIO_FOR_ONE_FRAME
  sf_close(sf);
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
       "int $128\n"                             \
                                                );
}
#else
int main(int argc, char *argv[]){
  main2();
  return 0;
}
#endif
