/** 
 * Here are the rendering routines for my 4k intro production..  Uses
 * the shaders in shader.c
 *
 * This will be just #included in the main file. I don't think it's
 * "good practice" but I find it the simplest approach to get the
 * thing compiled in small space.
 */

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

  unipar = oglGetUniformLocation(pid, "s");
  oglUniform1fv(unipar, 9, state);

  //  glEnable(GL_DEPTH_TEST);
  oglClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

  //sivu(10,-2);
  glRects( -1, -1, 1, 1 );


}

/** Render something that varies with time and "audio snapshot". */
static
void render_w_shaders(const synti2_synth *s, float ar){

  //oglMatrixMode(GL_PROJECTION);
  //oglLoadIdentity();
  /* The exe would be smaller, were screen size hard-coded.. 
     .. but I try to be nice.. */
  //oglFrustum(-ar,ar,-1.f,1.f,1.f,4.f);
  //oglMatrixMode(GL_MODELVIEW);
  //oglLoadIdentity();
  oglUseProgram(pid);

  render_scene(s);
}
