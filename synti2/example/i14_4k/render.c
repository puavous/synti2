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

#define NEED_DEBUG

/* Global now. Some bytes shorter code..: */
  GLfloat state[NUM_SYNTH_PARAMS];

/** Paint it. */
static void render_scene(const synti2_synth *s){
  int i, j;
  GLint unipar;
  GLfloat *isp;

  /*
     compression    old  new
     7za    -mx=9   4176 4244
     zopfli --i25   4110 4128

     Somehow this renderer gets compressed better
     than the previous one, and now we get all
     the envelopes, controllers, and notes for
     all voices into the shader side.
     
  */

  /* Global now:
     float synthtime;
     synthtime =  (float)(s->framecount) / s->sr;
  */
  //state[0] = synthtime;
  //state[1] = window_h*ar; /* globals */
  //state[2] = window_h;    /* globals */

  isp = state + NUM_GLOBAL_PARAMS;
  
  synti2_voice *v = s->voi;
  for(i=0; i<NUM_CHANNELS; i++){
    for(j=0; j<NUM_ENVS+1; j++)
      *isp++ = v->eprog[j].f;
    for(j=0; j<NUM_MODULATORS; j++)
      *isp++ = v->contr[j].f;
    *isp++ = v->note;
    v++;
  }

  unipar = oglGetUniformLocation(pid, "s");
  oglUniform1fv(unipar, NUM_SYNTH_PARAMS, state);

  //  glEnable(GL_DEPTH_TEST);
  oglClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

  glRects( -1, -1, 1, 1 );
}


/** Render something that varies with time and "audio snapshot". */
static
void render_w_shaders(const synti2_synth *s){
  oglUseProgram(pid);
  render_scene(s);
}
