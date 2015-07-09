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
 * graphics...
 */
#define NUM_SYNTH_PARAMS_TRANSMITTED 200

//#define NEED_DEBUG 1

/** Paint it. */
static void render_scene(const synti2_synth *s){
  int i, j;
  GLint unipar;
  GLfloat *isp;

  GLuint tex;

  #define TEXTURE_W 256
  #define TEXTURE_H 256

  static int first_time = 1;
  static float pixels[TEXTURE_W*TEXTURE_H*3];
  
  /* FIXME: static render_init() for all initialization */
  if (first_time){
      first_time = 0;
      /* Straight from an OpenGL intro at first... */
      oglGenTextures(1, &tex);
      oglBindTexture(GL_TEXTURE_2D, tex);
      oglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      oglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      oglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      oglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      //oglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      //oglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      int rs = 1;
      // Generate RGB noise:
      for (i=0;i<TEXTURE_W*TEXTURE_H*3;i++){
          rs *= 16807;
          pixels[i]=(float)((unsigned int)rs) * 4.6566129e-010f;
      }
      /*
      // Generate a test image in RGB:
      for (i=0;i<TEXTURE_W*TEXTURE_H*3;i+=3){
          pixels[i] = (float)((i/3)%TEXTURE_W)/(TEXTURE_H);
          pixels[i+1] = (float)((i/3)/TEXTURE_H)/(TEXTURE_W);
          pixels[i+2] = (float)(i/3)/(TEXTURE_W*TEXTURE_H);
      }
      */
      oglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_W, TEXTURE_H, 0, GL_RGB, GL_FLOAT, pixels);
  }

  /* Global now:
     float synthtime;
     synthtime =  (float)(s->framecount) / s->sr;
  */
  //state[0] = synthtime;
  //state[1] = window_h*ar; /* globals */
  //state[2] = window_h;    /* globals */

  isp = state + NUM_GLOBAL_PARAMS;
  
  // The whole state doesn't fit in uniform arrays of some gfx cards..
  #define NUM_CHANNELS_TO_TRANSFER 3
  
  const synti2_voice *v = s->voi;
  for(i=0; i<NUM_CHANNELS_TO_TRANSFER; i++){
    for(j=0; j<NUM_ENVS+1; j++)
      *isp++ = v->c[CI_ENVS+j].f;
    /* for consistency btw compose&playback*/
    isp += (NUM_MAX_ENVS - NUM_ENVS);
    for(j=0; j<NUM_MODULATORS; j++)
      *isp++ = v->c[CI_MODS+j].f;
    /* for consistency btw compose&playback*/
    isp += (NUM_MAX_MODULATORS - NUM_MODULATORS);
    *isp++ = v->note;
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
