/** 
 * Here are the rendering routines for my 4k intro production..  Uses
 * the shaders in shader.c
 *
 * This will be just #included in the main file. I don't think it's
 * "good practice" but I find it the simplest approach to get the
 * thing compiled in small space.
 */

static void sivu(int a, int z){
      oglBegin(GL_QUADS);

      /*glNormal3f(  0.f,   0.f,   1.f);*/
      oglVertex3i(  a,   a,   z);

      /*glNormal3f(  0.f,   0.f,   1.f);*/
      oglVertex3i( -a,   a,   z);

      /*glNormal3f(  0.f,   0.f,   1.f);*/
      oglVertex3i( -a,  -a,   z);

      /*glNormal3f(  0.f,   0.f,   1.f);*/
      oglVertex3i(  a,  -a,   z);
      oglEnd();
}

static void kuutio(int a){
  //oglPushMatrix();
  sivu(1,a);
  oglRotatef(90.f, 1.f, 0.f, 0.f); sivu(1,a);
  oglRotatef(90.f, 1.f, 0.f, 0.f); sivu(1,a);
  oglRotatef(90.f, 1.f, 0.f, 0.f); sivu(1,a);
  oglRotatef(90.f, 0.f, 1.f, 0.f); sivu(1,a);
  oglRotatef(180.f, 0.f, 1.f, 0.f); sivu(1,a);
  //oglPopMatrix();
}




/** Paint it. */
static void render_scene(const synti2_synth *s){
  int i, j;

  float cf;

  GLint unipar;
  GLfloat state[9];

  float synthtime;
  synthtime =  (float)(s->framecount) / s->sr;

  state[0] = synthtime;

  for(i=1;i<9;i++){
    state[i] = s->voi[i].eprog[1].f;
  }
  //cf = 1.0f+s->voi[9].eprog[1].f;
  //glClearColor (1.0f, 1.0f, 1.0f, 0.0);

  unipar = oglGetUniformLocation(pid, "s");
  oglUniform1fv(unipar, 9, state);
  //printf("%d  ",unipar);fflush(stdout);


  //glDisable(GL_DEPTH_TEST);
  oglEnable (GL_BLEND); 
  oglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  oglClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

  sivu(i,-i);
  /*
  sivu(i,-9);

    sivu(10,-8);
  */
  oglRotatef (synthtime*20.f, sin(synthtime), 0.2f, 0.f);

  oglEnable(GL_DEPTH_TEST);
  for(;i>=0;i--){
    kuutio(i);
  }
  /*kuutio(3);
    kuutio(2);
    kuutio(1);
    kuutio(0);*/
}

/** Render something that varies with time and "audio snapshot". */
static
void render_w_shaders(const synti2_synth *s, float ar){

  oglMatrixMode(GL_PROJECTION);
  oglLoadIdentity();
  /* The exe would be smaller, were screen size hard-coded.. 
     .. but I try to be nice.. */
  oglFrustum(-ar,ar,-1.f,1.f,4.f,400.f);
  oglMatrixMode(GL_MODELVIEW);
  oglLoadIdentity();
  oglUseProgram(pid);

  render_scene(s);
}
