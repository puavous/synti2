/** 
 * Let us see, what we can read from the synth instance directly...
 */
#include <math.h>
#include <stdlib.h>
#include "GL/gl.h"
#include "SDL/SDL.h"

#include "synti2_guts.h"

static void sivu(){
      glBegin(GL_QUADS);
      glVertex3i(  1,   1,   1);
      glVertex3i( -1,   1,   1);
      glVertex3i( -1,  -1,   1);
      glVertex3i(  1,  -1,   1);
      glEnd();
}

static void kuutio(){
  glPushMatrix();
  sivu();
  glRotatef(90.f, 1.f, 0.f, 0.f); sivu();
  glRotatef(90.f, 1.f, 0.f, 0.f); sivu();
  glRotatef(90.f, 1.f, 0.f, 0.f); sivu();
  glRotatef(90.f, 0.f, 1.f, 0.f); sivu();
  glRotatef(180.f, 0.f, 1.f, 0.f); sivu();
  glPopMatrix();
}


/** Paint a triangle :). */
static void render_scene(const synti2_synth *s){
  int i, j;

  float time = (float)(s->framecount) / s->sr;
  glClearColor(s->voi[0].eprog[1].f, 
               s->voi[1].eprog[1].f, 
               s->voi[2].eprog[1].f, 0.0f);

  //glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
  glClear(GL_DEPTH_BUFFER_BIT);

  glLoadIdentity();
  glTranslatef(0.f, -30.f ,-160.f); /* +100. * sin(time) - .1*s->note[0]); */
  /*glTranslatef(data[400]*time,data[600]*time,-150);*/
  
  glRotatef(time*45.f, 0.f, 1.f, 0.f);

  glScalef(4.f, 4.f, 4.f);

  glTranslatef(-24.f, 0.f, -6.f);
  for(i = 0; i<16; i++){
    for(j = 0; j<6; j++){
      glPushMatrix();
      glTranslatef(i*3.f, 0.f, j*3.f);
      glScalef(1.f, s->voi[i].eprog[j].f, 1.f);
      glTranslatef(0.f, 1.f, 0.f);
      kuutio();
      glPopMatrix();
    }
  }
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
