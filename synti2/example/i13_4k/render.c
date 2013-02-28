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

  //   GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
  //GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
  GLfloat mat_ambient[4];
  GLfloat mat_specular[4];
  GLfloat mat_shininess[] = { 50.0f };
  GLfloat light_position[] = { 1.0f, 1.0f, 1.0f, 0.0f };

  float cf;
  int a,n;

  for(i=0;i<4;i++){
    mat_specular[i] = 1.f - s->voi[2].eprog[3].f;
  }
   
   //   GLfloat mat_diffuse[] = { 1.0f, 0.0f, 0.0f, 1.0f };

   cf = s->voi[1].eprog[1].f;
   glClearColor (cf, cf, cf, 0.0);

   //   glShadeModel (GL_SMOOTH);  // Default?

   glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
   glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_specular);
   glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
   glLightfv(GL_LIGHT0, GL_POSITION, light_position);

   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_DEPTH_TEST);

   glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

  glLoadIdentity();
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);

  glTranslatef(0.f, 0.f , -20.f); // +100. * sin(time) - .1*s->note[0]);
  /*glTranslatef(data[400]*time,data[600]*time,-150);*/

  // glScalef(mat_specular[0], mat_specular[0],mat_specular[0]);


  //glRotatef(time*45.f, 0.f, 1.f, 0.f);
  glRotatef(time*(1.f+s->voi[4].note), 1.f, s->voi[4].note % 3, 0.f);

  a = s->voi[4].note % 13;
  n = 10 + (a);
  for(i = 0; i<n; i++){
    //    glPushMatrix();
    himpale(6, 10.f+cf);
    //    glPopMatrix();
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
  glEnable(GL_DEPTH_TEST);

  oglUseProgram(pid);
  render_scene(s);
  //SDL_GL_SwapBuffers();
}
/*
static
void render_w_shaders(const synti2_synth *s){
    oglUseProgram(pid);
    glRotatef(0.3f,1,1,1);
    glRects(-1,-1,1,1);
}

*/
