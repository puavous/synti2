/* It's a shorter code when these are linked in our own table.  .. But
* not all of these are always needed... So should make some automatic
* customization of these..
*
* FIXME: Hmm... Actually, to omit a function would cost only a couple
* of bytes now.. something like if(str[0]=='
* '){skip_one(str,dlfuncs);} so the definitions (.h) could actually be
* fixed, and only the string in glfuncs.c would change... Actual table
* should be renamed. It will be a global symbol with storage in .bss
*/
#ifndef GLFUNCS_H
#define GLFUNCS_H

#include "GL/glew.h"
#include "GL/gl.h"
#include "GL/glx.h"
#include "SDL/SDL.h"

typedef void *func_t(void);

extern const char funs[];

typedef void (*MATMODE_F_T)(GLenum);
typedef void (*FRUSTUM_F_T)(GLdouble left, GLdouble right, 
                            GLdouble bottom, GLdouble top,
                            GLdouble near_val, GLdouble far_val );
typedef void (*ROTATE_F_T)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z );
typedef void (*BEGIN_F_T)(GLenum mode);
typedef void (*ENABLE_F_T)(GLenum cap);
typedef void (*CLEAR_F_T)(GLbitfield mask);
typedef void (*RECTS_F_T)(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
typedef void (*VERTEX3I_F_T)(GLint x, GLint y, GLint z);
typedef void (*TRANSLATEF_F_T)(GLfloat tx, GLfloat ty, GLfloat tz);
typedef void (*BLENDFUNC_F_T)( GLenum sfactor, GLenum dfactor);
typedef GLint (*GETUNIFORMLOC_F_T)( GLuint program, const GLchar *name);
typedef void (*UNIFORM1FV_F_T)( GLint location, GLsizei count, const GLfloat *value);

typedef void (*SDLINIT_F_T)(Uint32 flags);
typedef SDL_Surface * (*SDLSETV_F_T)( int w, int h, int bpp, Uint32 flags );
typedef int (*SDLSHOW_F_T) (int toggle);
typedef int (*SDLOPAU_F_T) (SDL_AudioSpec *desired, SDL_AudioSpec *obtained);
typedef void (*SDLPAUS_F_T) (int pause_on);
typedef void (*SDLGLSW_F_T) (void);
typedef void (*SDLPOLL_F_T) (SDL_Event *event);
typedef void (*SDLQUIT_F_T)( void );

/* functions from libGL.so */
#define oglCreateProgram	  ((PFNGLCREATEPROGRAMPROC)myglfunc[0])
#define oglCreateShader		  ((PFNGLCREATESHADERPROC) myglfunc[1])
#define oglShaderSource     ((PFNGLSHADERSOURCEPROC) myglfunc[2])
#define oglCompileShader    ((PFNGLCOMPILESHADERPROC)myglfunc[3])
#define oglAttachShader     ((PFNGLATTACHSHADERPROC) myglfunc[4])
#define oglLinkProgram      ((PFNGLLINKPROGRAMPROC)  myglfunc[5])
#define oglUseProgram       ((PFNGLUSEPROGRAMPROC)   myglfunc[6])
#define oglClear            ((CLEAR_F_T)             myglfunc[7])
#define oglRects            ((RECTS_F_T)             myglfunc[8])
#define oglGetUniformLocation ((GETUNIFORMLOC_F_T)   myglfunc[9])
#define oglUniform1fv       ((UNIFORM1FV_F_T)        myglfunc[10])

/* functions from libSDL.so */
#define oSDL_Init           ((SDLINIT_F_T)          myglfunc[11])
#define oSDL_SetVideoMode   ((SDLSETV_F_T)          myglfunc[12])
#define oSDL_ShowCursor     ((SDLSHOW_F_T)          myglfunc[13])
#define oSDL_OpenAudio      ((SDLOPAU_F_T)          myglfunc[14])
#define oSDL_PauseAudio     ((SDLPAUS_F_T)          myglfunc[15])
#define oSDL_GL_SwapBuffers ((SDLGLSW_F_T)          myglfunc[16])
#define oSDL_PollEvent      ((SDLPOLL_F_T)          myglfunc[17])
#define oSDL_Quit           ((SDLQUIT_F_T)          myglfunc[18])

#define NUMFUNCTIONS 100    //number of functions in *strs function array

#endif
