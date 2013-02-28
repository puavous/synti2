/** 
 * Here are the rendering routines for my 4k intro production..  Uses
 * the shaders in shader.c
 *
 * This will be just #included in the main file. I don't think it's
 * "good practice" but I find it the simplest approach to get the
 * thing compiled in small space.
 */

static void sivu(){
      glBegin(GL_QUADS);
      glNormal3f(  0.f,   0.f,   1.f);
      glVertex3i(  1,   1,   1);

      glNormal3f(  0.f,   0.f,   1.f);
      glVertex3i( -1,   1,   1);

      glNormal3f(  0.f,   0.f,   1.f);
      glVertex3i( -1,  -1,   1);

      glNormal3f(  0.f,   0.f,   1.f);
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

static void himpale(int par1, float par2){
  int i;
  glPushMatrix();
  for(i=0;i<par1;i++){
    glRotatef(360.f/par1, 0.f, 0.f, 1.f);
    glPushMatrix();
    glTranslatef(par2, 0.f, 0.f);
    kuutio();
    glPopMatrix();
  }
  glPopMatrix();
}


/** Paint it. */
static void render_scene(const synti2_synth *s){
  int i, j;

  float time = (float)(s->framecount) / s->sr;

  float cf;
  int a,n;

  cf = s->voi[9].eprog[1].f;
  glClearColor (cf, cf, cf, 0.0);

  glEnable(GL_DEPTH_TEST);
  glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

  glLoadIdentity();

  //glTranslatef(0.f, 0.f , -20.f);

  glRotatef(time*(1.f+s->voi[4].note), 1.f, s->voi[4].note % 3, 0.f);

  a = s->voi[4].note % 13;
  n = 10 + (a);
  for(i = 0; i<n; i++){
    himpale(6, 10.f+cf);
    glTranslatef(0.f, 0.f, 3.f-cf);
    glRotatef(360.f/n, 1.f, 0.f, 0.f);
  }
}

/** Render something that varies with time and "audio snapshot". */
void render_w_shaders(const synti2_synth *s){
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-1.33,1.33,-1,1,1.5,400);

  glMatrixMode(GL_MODELVIEW);
  /*  glEnable(GL_DEPTH_TEST);*/

  oglUseProgram(pid);

  render_scene(s);

  /*SDL_GL_SwapBuffers();*/
}
/*
static
void render_w_shaders(const synti2_synth *s){
    oglUseProgram(pid);
    glRotatef(0.3f,1,1,1);
    glRects(-1,-1,1,1);
}

*/
