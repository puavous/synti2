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

static void himpale(int par1, int par2){
  int i;
  glPushMatrix();
  for(i=0;i<par1;i++){
    glRotatef(360/par1, 0.f, 0.f, 1.f);
    glPushMatrix();
    glTranslatef(1.5f, 0.f, 0.f);
    kuutio();
    glPopMatrix();
  }
  glPopMatrix();
}

#if 0
static void objekti(unsigned int par){
  if (par==0) return;
  glRotatef(25.f, 0.f, 1.f, 0.f);
  glTranslatef(0.f, 7.f, 0.f);
  glPushMatrix();
  glScalef(1.f, 8.f, 1.f);
  kuutio();
  glPopMatrix();
  glScalef(.96f, .98f, .95f);

  glPushMatrix();

  switch(par & 0x7){
  case 1:
    glRotatef(65.f, 1.f, 0.f, 0.f);
    glPushMatrix();
    objekti(par / 2);
    glPopMatrix();
    break;
  case 2:
    glRotatef(-40.f, 1.f, 0.f, 0.f);
    glPushMatrix();
    objekti(par / 5);
    glPopMatrix();
    break;
    /*
  case 3:
    glRotatef(-35.f, 0.f, 0.f, 1.f);
    glPushMatrix();
    objekti(par / 7);
    glPopMatrix();
    break;
  case 4:
    glRotatef(35.f, 0.f, 0.f, 1.f);
    glPushMatrix();
    objekti(par / 5);
    glPopMatrix();
    break;
    */
  }

  glPopMatrix();
  objekti(par/2);

  //if (par & 1) 
  //glRotatef(20.f, 0.f, 1.f, 0.f);
  //par >>= 1;
  //objekti(par);
}
#endif

/** Paint a triangle :). */
static void render_scene(const synti2_synth *s){
  int i, j;

  float amb[4];
  float spec[4];

  float time = (float)(s->framecount.val) / s->sr;

  /*
  glClearColor(s->eprog[0][1].f, 
               s->eprog[1][1].f, 
               s->eprog[2][1].f, 0.0f);
  */

  glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

  glEnable(GL_LIGHTING);

  amb[0] = 1.f-s->eprog[0][1].f;
  amb[1] = 1.f-s->eprog[1][1].f;
  amb[2] = 1.f-s->eprog[2][1].f;
  amb[3] = s->eprog[3][1].f;

  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);


  //  glLightfv(GL_LIGHT0, GL_SPECULAR, specular);





  glLoadIdentity();
  glTranslatef(0.f, -30.f ,-200.f); // +100. * sin(time) - .1*s->note[0]);
  /*glTranslatef(data[400]*time,data[600]*time,-150);*/

  
  glRotatef(time*45.f, 0.f, 1.f, 0.f);

  glScalef(10.f, 10.f, 10.f);

  glPushMatrix();
  himpale(6, 10);
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
