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

static void recurs(int n, float am){
  if (n==0) return;
  primit();
  glRotatef(am, 1.f, am , 2*n);
  glTranslatef(1.f,-1.f,0.f);
  //  glScalef(1.01f,.99f,.99f);
  recurs(n/2,am);
  recurs(n/2,-am*2.f);
}

static void morko(int par, float par2){
  int j;
  for(j=0; j<9; j++){
      
    glRotatef(40, 0.f,1.f,2.f);
    //glRotatef(40, 1.f, 0.f, 0.f);
    glPushMatrix();
    recurs(par, par2);
    glPopMatrix();

  }
}


/** Paint a triangle :). */
static void render_scene(const synti2_synth *s){
  int i, j;

  float time = (float)(s->framecount.val) / s->sr;
  glClearColor(s->eprog[0][1].f, 
               s->eprog[1][1].f, 
               s->eprog[2][1].f, 0.0f);

  glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

  glLoadIdentity();
  glTranslatef(0.f, 10.f ,-160.f); // +100. * sin(time) - .1*s->note[0]);
  /*glTranslatef(data[400]*time,data[600]*time,-150);*/
  
  glRotatef(time*180.f, 0.f, 1.f, 0.f);

  glScalef(10.f, 10.f, 10.f);

  kuutio();

  glTranslatef(3.f, 0.f, 0.f);

  glPushMatrix();
  glScalef(1.f, s->eprog[0][1].f, 1.f);
  kuutio();
  glPopMatrix();

  glTranslatef(3.f, 0.f, 0.f);

  glPushMatrix();
  glScalef(1.f, s->eprog[0][2].f, 1.f);
  kuutio();
  glPopMatrix();



  //  glPopMatrix();
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
