/**
 * Here are the rendering routines for my 4k intro production..  Uses
 * the shaders in shader.c
 *
 * This will be just #included in the main file. I don't think it's
 * "good practice" but I find it the simplest approach to get the
 * thing compiled in small space.
 */

/* Some gfx cards don't allow very many uniform variables, so send
 * fewer than available.. use the first few channels to control
 * graphics... TODO: Send as matrices or vectors instead of floats!
 */
#define NUM_SYNTH_PARAMS_TRANSMITTED 200

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

  const synti2_voice *v = s->voi;
  for(i=0; i<NUM_CHANNELS; i++){
      // FIXME: Hack...
      *isp++ = v->c[CI_ENVS+1].f;
      *isp++ = v->note;
#if 0
  for(i=0; i<NUM_CHANNELS; i++){
    for(j=0; j<NUM_ENVS+1; j++)
      *isp++ = v->c[CI_ENVS+j].f;
    /* for consistency btw compose&playback*/
    isp += (NUM_MAX_ENVS - NUM_ENVS);
    for(j=0; j<NUM_MODULATORS; j++)
      *isp++ = v->c[CI_MODS+j].f;
    /* for consistency btw compose&playback*/
    isp += (NUM_MAX_MODULATORS - NUM_MODULATORS);
    *isp++ = v->note;
#endif
    v++;
  }

  unipar = oglGetUniformLocation(pid, "s");

  oglUniform1fv(unipar, NUM_SYNTH_PARAMS_TRANSMITTED, state);

  oglClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

  oglRects( -1, -1, 1, 1 );
}


/** Render something that varies with time and "audio snapshot". */
static
void render_w_shaders(const synti2_synth *s){
  oglUseProgram(pid);
  render_scene(s);
}
