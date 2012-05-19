/** 
 * Let us see, what we can read from the synth instance directly...
 */
#include <math.h>
#include <stdlib.h>
#include "GL/gl.h"
#include "SDL/SDL.h"

#include "synti2_guts.h"

static void face(){
  glBegin(GL_TRIANGLES);
  glNormal3f(-0.90451f,   0.29390f,   0.58779f);
  glVertex3f(  0.f,   1.f,   .0f);

  glNormal3f(-0.90451f,   0.29390f,   0.58779f);
  glVertex3f( -.58779f,   -.80902f,   0.f);

  glNormal3f(-0.90451f,   0.29390f,   0.58779f);
  glVertex3f(  0.f,  0.f,    .5f);
  glEnd();
}

static void star(){
  glPushMatrix();
  face();
  glRotatef(360.f/5, 0.f, 0.f, 1.f); face();
  glRotatef(360.f/5, 0.f, 0.f, 1.f); face();
  glRotatef(360.f/5, 0.f, 0.f, 1.f); face();
  glRotatef(360.f/5, 0.f, 0.f, 1.f); face();

  glRotatef(180.f, 0.f, 1.f, 0.f);

  face();
  glRotatef(360.f/5, 0.f, 0.f, 1.f); face();
  glRotatef(360.f/5, 0.f, 0.f, 1.f); face();
  glRotatef(360.f/5, 0.f, 0.f, 1.f); face();
  glRotatef(360.f/5, 0.f, 0.f, 1.f); face();


  glPopMatrix();
}

static void himpale(int par1, float par2){
  int i;
  glPushMatrix();
  for(i=0;i<par1;i++){
    glRotatef(360.f/par1, 0.f, 0.f, 1.f);
    glPushMatrix();
    glTranslatef(par2, 0.f, 0.f);
    star();
    /*    glRotatef(180.f, 0.f, 1.f, 0.f);
    star();
    */
    glPopMatrix();
  }
  glPopMatrix();
}


/** Paint a triangle :). */
static void render_scene(const synti2_synth *s){
  int i, j;

  float time = (float)(s->framecount.val) / s->sr;

  //   GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
  //GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
  GLfloat mat_ambient[4];
  GLfloat mat_specular[4];
  for(i=0;i<4;i++){
    mat_specular[i] = 1.f - s->eprog[2][3].f;
  }
   
   //   GLfloat mat_diffuse[] = { 1.0f, 0.0f, 0.0f, 1.0f };
   GLfloat mat_shininess[] = { 50.0f };
   GLfloat light_position[] = { 1.0f, 1.0f, 1.0f, 0.0f };

   float cf = s->eprog[1][1].f;
   glClearColor (cf, cf, cf, 0.0);

   //   glShadeModel (GL_SMOOTH);  // Default?

   glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
   glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_specular);
   glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
   glLightfv(GL_LIGHT0, GL_POSITION, light_position);

   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);

   glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

  glLoadIdentity();
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);

  glTranslatef(0.f, 0.f , -20.f); // +100. * sin(time) - .1*s->note[0]);
  glRotatef(90.f, 0.f, 0.f , 1.f); // +100. * sin(time) - .1*s->note[0]);
  /*glTranslatef(data[400]*time,data[600]*time,-150);*/

  // glScalef(mat_specular[0], mat_specular[0],mat_specular[0]);


  //glRotatef(time*45.f, 0.f, 1.f, 0.f);
  glRotatef(time*(1.f+s->note[4]), 1.f, s->note[4] % 3, 0.f);

  int a = s->note[4] % 13;

  int n = 10 + (a);
  for(i = 0; i<n; i++){
    //    glPushMatrix();
    himpale(6, 10.f+cf);
    //    glPopMatrix();
    glTranslatef(0.f, 0.f, 3.f-cf);
    glRotatef(360.f/n, 1.f, 0.f, 0.f);
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
