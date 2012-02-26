/** 
 * Let us see, what we can read from the synth instance directly...
 */
#include <math.h>
#include <stdlib.h>
#include "GL/gl.h"
#include "SDL/SDL.h"

#include "synti2_guts.h"

#define NSTRIPS 25

/** Paint a triangle :). */
static void render_scene(const synti2_synth *s){
  int i, j;

  float time = (float)(s->framecount.val) / s->sr;
  glClearColor(s->eprog[0][1].f, 
               s->eprog[1][1].f, 
               s->eprog[2][1].f, 0.0f);

  glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

  glEnable(GL_BLEND);

  glLoadIdentity();
  glTranslatef(0.,0.,-150 +100. * sin(time) + 50*s->eprog[0][1].f);
  /*glTranslatef(data[400]*time,data[600]*time,-150);*/
  glRotatef(time*3,0.,1.,0.);
  glRotatef(time*7,0.,0.,1.);

  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  
  glPushMatrix();
  for(j=0; j<NSTRIPS; j++){
    glPopMatrix();

    glRotatef(180.+22.5,0.,1.,0.);
    glRotatef(j*15,1.,0.,0.);

    glPushMatrix();

    for(i=0; i<120; i++){

      glTranslatef(1. /*+ sin(time)*/,0.,0.);
      /*
      glColor4f(.3+data[((i+0))%dlen], 
                .3+data[((i+100))%dlen], 
                .3+data[((i+200))%dlen], 8.0/i);
      */

      /* The primitive: a tri-angleee */
      glBegin(GL_TRIANGLES);
      glVertex3f( 0,   1, 0);
      glVertex3f( 1,  -1, 0);
      glVertex3f(-2,  -1, i/8);
      glEnd();
    }
  }
  glPopMatrix();
}

/** Render something that varies with time and "audio snapshot". */
void render_using_synti2(const synti2_synth *s){
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-1.33,1.33,-1,1,1.5,400);

  glMatrixMode(GL_MODELVIEW);
  glEnable(GL_DEPTH_TEST);

  render_scene(s);
  SDL_GL_SwapBuffers();
}
