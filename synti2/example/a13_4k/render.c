/** 
 * Here are the rendering routines for my 4k intro production..  Uses
 * the shaders in shader.c
 *
 * This will be just #included in the main file. I don't think it's
 * "good practice" but I find it the simplest approach to get the
 * thing compiled in small space.
 */

#define NUM_GLOBAL_PARAMS 3
#define NUM_SYNTH_PARAMS 9

/** Paint it. */
static void render_scene(const synti2_synth *s){
  int i, j;

  float cf;

  GLint unipar;
  GLfloat state[NUM_GLOBAL_PARAMS + NUM_SYNTH_PARAMS];

  float synthtime;
  synthtime =  (float)(s->framecount) / s->sr;

  state[0] = synthtime;

#ifndef NO_FULLSCREEN
  state[1] = vid->current_w; /* global struc now */
  state[2] = vid->current_h; /* global struc now */
#else
  state[1] = window_h*ar; /* globals */
  state[2] = window_h;    /* globals */
#endif



  for(i=NUM_GLOBAL_PARAMS;
      i<NUM_GLOBAL_PARAMS + NUM_SYNTH_PARAMS;
      i++){
    state[i] = s->voi[i].eprog[1].f;
  }

  unipar = oglGetUniformLocation(pid, "s");
  oglUniform1fv(unipar, NUM_GLOBAL_PARAMS + NUM_SYNTH_PARAMS, state);

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
