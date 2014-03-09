/** 
 * Here are the rendering routines for my 4k intro production..  Uses
 * the shaders in shader.c
 *
 * This will be just #included in the main file. I don't think it's
 * "good practice" but I find it the simplest approach to get the
 * thing compiled in small space.
 */

#define NUM_GLOBAL_PARAMS 3
#define NUM_SYNTH_PARAMS (NUM_GLOBAL_PARAMS + NUM_CHANNELS * (NUM_ENVS + 1 + NUM_MODULATORS + 1))

/* Global now. Some bytes shorter code..: */
  GLfloat state[NUM_SYNTH_PARAMS];

/** Paint it. */
static void render_scene(const synti2_synth *s){
  int i, j;

  float cf;

  GLint unipar;


  int isp;

  /* Global now:
     float synthtime;
     synthtime =  (float)(s->framecount) / s->sr;
  */

  isp = 3;
  
  //state[0] = synthtime;
  //state[1] = window_h*ar; /* globals */
  //state[2] = window_h;    /* globals */

#if 0
  synti2_voice *v = s->voi;
  for(i=0; i<NUM_CHANNELS; i++){
    for(j=0; j<NUM_ENVS+1; j++)
      state[isp++] = v->eprog[j].f;
    for(j=0; j<NUM_MODULATORS; j++)
      state[isp++] = v->contr[j].f;
    state[isp++] = v->note;
    v++;
  }
#endif

  for(i=0; i<NUM_CHANNELS; i++){
    for(j=0;j<NUM_ENVS+1;j++){
      state[isp++] = s->voi[i].eprog[j].f;
    }
    for(j=0;j<NUM_MODULATORS;j++){
      state[isp++] = s->voi[i].contr[j].f;
    }
    state[isp++] = s->voi[i].note;    
  }


  unipar = oglGetUniformLocation(pid, "s");
  oglUniform1fv(unipar, NUM_SYNTH_PARAMS, state);

  //  glEnable(GL_DEPTH_TEST);
  oglClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

  //sivu(10,-2);
  glRects( -1, -1, 1, 1 );

}

/** Render something that varies with time and "audio snapshot". */
static
void render_w_shaders(const synti2_synth *s){

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
