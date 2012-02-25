/** 
 * Graphics of "Teh 4k 3000" pikkiriikkinen demo at Instanssi 2011.  I
 * couldn't do a very stunning thing in the short time and space
 * available after creating the software synthesizer and
 * sequencer. But we have some motion with colours. Lots of hard-coded
 * stuff here, sorry.
 */
#include <math.h>
#include <stdlib.h>
#include "GL/gl.h"
#include "SDL/SDL.h"
#define NSTRIPS 25

/** Paint a triangle :). */
static void render_scene(float time, float *data, int dlen){
  int i, j;

  glEnable(GL_BLEND);

  glLoadIdentity();
  glTranslatef(0.,0.,-150.);
  /*glTranslatef(data[400]*time,data[600]*time,-150);*/
  glRotatef((time*(10.+data[200]*80.)),0.,1.,0.);
  glRotatef((time*data[100]*80.),0.,0.,1.);

  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  
  glPushMatrix();
  for(j=0; j<NSTRIPS; j++){
    glPopMatrix();

    glRotatef(180.+22.5,0.,1.,0.);
    glRotatef(j*15,1.,0.,0.);

    glPushMatrix();

    for(i=0; i<80; i++){

      glTranslatef(1. /*+ sin(time)*/,0.,0.);

      glColor4f(.3+data[((i+0))%dlen], 
                .3+data[((i+100))%dlen], 
                .3+data[((i+200))%dlen], 8.0/i);
      
      /* The primitive: a tri-angleee */
      glBegin(GL_TRIANGLES);
      glVertex3f( 0,   1+i*data[(4*i)%dlen], 0);
      glVertex3f( 1,  -1+i*data[(4*i)%dlen], 0);
      glVertex3f(-2,  -1+i*data[(4*i)%dlen], i/8);
      glEnd();
    }
  }
  glPopMatrix();
}

/** Render something that varies with time and "audio snapshot". */
void teh4k_render_at_time(float time, float *snapshot, size_t AUDIOBUFSIZE){
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-1.33,1.33,-1,1,1.5,400);

  glMatrixMode(GL_MODELVIEW);
  glEnable(GL_DEPTH_TEST);
  glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

  render_scene(time, snapshot, AUDIOBUFSIZE);
  if (time>30){
    render_scene(time*snapshot[10], snapshot, AUDIOBUFSIZE);
  }
  /*
  if (time>45){
    render_scene(time*snapshot[20], snapshot, AUDIOBUFSIZE, stride);
  }
  */
  SDL_GL_SwapBuffers();
}
