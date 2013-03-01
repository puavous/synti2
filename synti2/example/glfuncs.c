/* It's a shorter code when these are linked in our own table.  .. But
* not all of these are always needed... So should make some automatic
* customization of these..
*/

#define oglCreateProgram	            ((PFNGLCREATEPROGRAMPROC)myglfunc[0])
#define oglCreateShader		            ((PFNGLCREATESHADERPROC)myglfunc[1])
#define oglShaderSource                 ((PFNGLSHADERSOURCEPROC)myglfunc[2])
#define oglCompileShader                ((PFNGLCOMPILESHADERPROC)myglfunc[3])
#define oglAttachShader                 ((PFNGLATTACHSHADERPROC)myglfunc[4])
#define oglLinkProgram                  ((PFNGLLINKPROGRAMPROC)myglfunc[5])
#define oglUseProgram                   ((PFNGLUSEPROGRAMPROC)myglfunc[6])

typedef void (*MATMODE_F_T)(GLenum);
typedef void (*FRUSTUM_F_T)(GLdouble left, GLdouble right, 
                            GLdouble bottom, GLdouble top,
                            GLdouble near_val, GLdouble far_val );

#define oglLoadIdentity                 (myglfunc[7])
#define oglMatrixMode                   ((MATMODE_F_T)myglfunc[8])
#define oglFrustum                      ((FRUSTUM_F_T)myglfunc[9])

typedef void (*ROTATE_F_T)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z );
typedef void (*BEGIN_F_T)(GLenum mode);
typedef void (*ENABLE_F_T)(GLenum cap);
typedef void (*CLEAR_F_T)(GLbitfield mask);
typedef void (*VERTEX3I_F_T)(GLint x, GLint y, GLint z);
typedef void (*TRANSLATEF_F_T)(GLfloat tx, GLfloat ty, GLfloat tz);

#define oglRotatef      ((ROTATE_F_T)myglfunc[10])
#define	oglPushMatrix   (myglfunc[11])
#define	oglPopMatrix    (myglfunc[12])
#define	oglEnd          (myglfunc[13])
#define oglBegin        ((BEGIN_F_T)myglfunc[14])
#define oglEnable       ((ENABLE_F_T)myglfunc[15])
#define oglClear        ((CLEAR_F_T)myglfunc[16])
#define oglVertex3i     ((VERTEX3I_F_T)myglfunc[17])
//#define oglTranslatef   ((TRANSLATEF_F_T)myglfunc[18])
#define NUMFUNCTIONS 18    //number of functions in *strs function array

static char *strs[] = {
	"glCreateProgram",
	"glCreateShader",
	"glShaderSource",
	"glCompileShader",
	"glAttachShader",
	"glLinkProgram",
	"glUseProgram",

	"glLoadIdentity",
	"glMatrixMode",
	"glFrustum",

	"glRotatef",
	"glPushMatrix",
	"glPopMatrix",
	"glEnd",
  "glBegin",
  "glEnable",
  "glClear",
  "glVertex3i",
  //  "glTranslatef",
};
