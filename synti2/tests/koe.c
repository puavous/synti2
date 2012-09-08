/** Yep. I think this should work, but on my Fedora16 system, the
 * Intel graphics driver crashes inside glCreateProgram();
 *
 * So I won't be coding any shaders before this is sorted out.. If
 * it's not magically fixed in F17, I'll have to make a bug report, I
 * guess..
 */
#include "GL/gl.h"
int main(int argc, char **argv){
  GLuint pid;
  pid = glCreateProgram();
  return 0;
}
