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
#define AUDIOBUFSIZE  4096

#define NUMFUNCTIONS 7    //number of functions in *strs function array

#define oglCreateProgram	            ((PFNGLCREATEPROGRAMPROC)myglfunc[0])
#define oglCreateShader		            ((PFNGLCREATESHADERPROC)myglfunc[1])
#define oglShaderSource                 ((PFNGLSHADERSOURCEPROC)myglfunc[2])
#define oglCompileShader                ((PFNGLCOMPILESHADERPROC)myglfunc[3])
#define oglAttachShader                 ((PFNGLATTACHSHADERPROC)myglfunc[4])
#define oglLinkProgram                  ((PFNGLLINKPROGRAMPROC)myglfunc[5])
#define oglUseProgram                   ((PFNGLUSEPROGRAMPROC)myglfunc[6])

static char *strs[] = {
	"glCreateProgram",
	"glCreateShader",
	"glShaderSource",
	"glCompileShader",
	"glAttachShader",
	"glLinkProgram",
	"glUseProgram",
};

typedef void *func_t(void);

func_t *myglfunc[NUMFUNCTIONS];

/* stereo interleaved.. so SDL should have samples==AUDIOBUFSIZE/2 and
   bytes==AUDIOBUFSIZE / 4 for 16bit dynamic range (correct?)  */
float audiobuf[AUDIOBUFSIZE];

synti2_synth my_synth;

static long frame = 0;

/* These datas are created by the tool programs: */
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
  float vol = 32767.0f; /* Synti2 waveshaper squashes output to [-1,1] */
  
  /* Call the synth engine and convert samples to native type (SDL) */
  /* Lengths are now hacked - will have stereo output from synti2.*/
  synti2_render(&my_synth, 
		audiobuf, len/4); /* 4 = 2 bytes times 2 channels. */

  frame += len/4;
  for(i=0;i<len/2;i+=2){
#ifdef NO_STEREO
    ((Sint16*)stream)[i+1] 
      = (((Sint16*)stream)[i+0] = (Sint16)(audiobuf[i+0]*vol)); 
#else
    ((Sint16*)stream)[i+0] = (Sint16)(audiobuf[i+0]*vol); 
    ((Sint16*)stream)[i+1] = (Sint16)(audiobuf[i+1]*vol);
#endif
  }
}


/** Try to wrap it... */
static void main2(){
  SDL_Event event;
  SDL_AudioSpec aud;

  const SDL_VideoInfo * vid;
  float tnow;

  // gl shader stuff...
  GLuint vsh,fsh,pid;

  int i;

  /* Checks of possible failures?*/
  //st = synti2_create(MY_SAMPLERATE, patch_sysex, hacksong_data);
  synti2_init(&my_synth, MY_SAMPLERATE, patch_sysex, hacksong_data);

  /* Do some SDL init stuff.. */
  SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER);

  /* The shader here are stolen from somewhere and then re-stolen by me;
     they are here just as an example/placeholder*/
const GLchar *vs="\
varying vec4 p;\
void main(){\
p=sin(gl_ModelViewMatrix[1]*9.0);\
gl_Position=gl_Vertex;\
}";

const GLchar *fs= "\
varying vec4 p;\
void main(){\
float r,t,j;\
vec4 v=gl_FragCoord/400.0-1.0;\
r=v.x*p.r;\
for(int j=0;j<7;j++){\
t=v.x+p.r*p.g;\
v.x=t*t-v.y*v.y+r;\
v.y=p.g*3.0*t*v.y+v.y;\
}\
gl_FragColor=vec4(mix(p,vec4(t),max(t,v.x)));\
}";

  /* init external gl commands

     Note to self... there was a crash with 64-bit libraries, but
     things worked well with 32-bit lib. And I was puzzled for a
     long time, and so were others who tried to help..
     But, silly me, I'm coding C now... functions return an integer 
     (32 bits) by default, and I was lacking the signature which is
     found in "GL/glx.h". Then it was implicitly converted to an address
     (64 bits), so *no wonder* it crashed. So, the note to sel would be:
     add the "warnings as errors" compiler option as the first thing, when
     starting a project in C...
   */
  for(i=0; i<NUMFUNCTIONS;i++)
    {
      myglfunc[i] = glXGetProcAddress( (const unsigned char *)strs[i] );
#if 0
      printf("Func %d at: %lx  (\"%s\")\n",i, myglfunc[i],strs[i]);
      if( !myglfunc[i] )
        return(0);
#endif
    }

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


  /* It costs 23-35 bytes (compressed) to politely query the display
   * mode. But it is definitely worth the ease! Oooh, but it won't
   * work with a dual monitor setup! (tries to make fullscreen and
   * crashes, afaik) So, probably we'll have to make an option for
   * this... or just compile two versions, One for fullscreen and one
   * for windowed mode.
   */

#ifndef NO_FULLSCREEN
  vid = SDL_GetVideoInfo();  /* get desktop mode */
  SDL_SetVideoMode(vid->current_w, vid->current_h, 32,
		   SDL_OPENGL|SDL_FULLSCREEN);
#else
  SDL_SetVideoMode(800,600,32,SDL_OPENGL);
#endif

#ifndef ULTRASMALL
  SDL_WM_SetCaption("Soft synth SDL interface",0);
  SDL_ShowCursor(SDL_DISABLE);
#endif

  /* They say that OpenAudio needs to be called after video init. */
  aud.freq     = MY_SAMPLERATE;
  aud.format   = AUDIO_S16SYS;
  aud.channels = 2;
  aud.samples  = AUDIOBUFSIZE/aud.channels;  /* "samples" means frames */
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


  /* Start audio after inits are done.. */
  SDL_PauseAudio(0);

  do
  {
    render_using_synti2(&my_synth);
    SDL_PollEvent(&event);
  } while (event.type!=SDL_KEYDOWN); // && tnow <70.0);

  /* Hmm... What happens if I don't close these? Will the world collapse!?*/
#ifndef ULTRASMALL
  SDL_CloseAudio();  /* Hmm.. Does SDL_Quit() do what this does? */
#endif

  SDL_Quit();  /* This must happen. Otherwise problems with exit! */

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

  main2();

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
