#include "SDL/SDL.h"
#include "GL/gl.h"

#define DEMO_TIME 10000

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

void *myglfunc[NUMFUNCTIONS];

/* shaders. to be defined elsewhere */
extern const GLchar *vs;
extern const GLchar *fs;

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

#ifdef TINY
  SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER);  
#endif

  SDL_Event e;
  SDL_SetVideoMode(1366,768,32,SDL_OPENGL);

#ifndef TINY
  SDL_ShowCursor(SDL_DISABLE);
#endif

  int time, quit=0, start=SDL_GetTicks();
  unsigned int vsh,fsh,pid;


  // init external gl commands
  int i;
  for(i=0; i<NUMFUNCTIONS;i++)
    {
      myglfunc[i] = glXGetProcAddress( (const unsigned char *)strs[i] );
#ifndef TINY
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

   while(!quit)
    {
      time = SDL_GetTicks() - start;
      
      oglUseProgram(pid);
      glRotatef(0.3f,1,1,1);
      glRects(-1,-1,1,1);
      
      SDL_GL_SwapBuffers();
      
      if(time >= DEMO_TIME)
       quit=1;
      
      while(SDL_PollEvent(&e)>0)
	{
	  if(e.type==SDL_KEYDOWN)
	    quit=1;
	}
    }

#ifndef TINY
   SDL_Quit();
#endif

  // this assebler piece to hinder the default glibc ending code
  asm ( \
       "movl $1,%eax\n" \
       "xor %ebx,%ebx\n" \
       "int $128\n" \
      );
}
