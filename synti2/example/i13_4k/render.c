/** 
 * Here are the rendering routines for my 4k intro production..  Uses
 * the shaders in shader.c
 *
 * This will be just #included in the main file. I don't think it's
 * "good practice" but I find it the simplest approach to get the
 * thing compiled in small space.
 */

static void sivu(){
      oglBegin(GL_QUADS);
      /*glNormal3f(  0.f,   0.f,   1.f);*/
      oglVertex3i(  1,   1,   1);

      /*glNormal3f(  0.f,   0.f,   1.f);*/
      oglVertex3i( -1,   1,   1);

      /*glNormal3f(  0.f,   0.f,   1.f);*/
      oglVertex3i( -1,  -1,   1);

      /*glNormal3f(  0.f,   0.f,   1.f);*/
      oglVertex3i(  1,  -1,   1);
      oglEnd();
}

static void kuutio(){
  oglPushMatrix();
  sivu();
  oglRotatef(90.f, 1.f, 0.f, 0.f); sivu();
  oglRotatef(90.f, 1.f, 0.f, 0.f); sivu();
  oglRotatef(90.f, 1.f, 0.f, 0.f); sivu();
  oglRotatef(90.f, 0.f, 1.f, 0.f); sivu();
  oglRotatef(180.f, 0.f, 1.f, 0.f); sivu();
  oglPopMatrix();
}


/** Paint it. */
static void render_scene(const synti2_synth *s){
  int i, j;

  float time = (float)(s->framecount) / s->sr;

  float cf;
  int a,n;

  cf = 1.0f+s->voi[9].eprog[1].f;
  glClearColor (cf, cf, cf, 0.0);

  oglEnable (GL_BLEND); 
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  //oglEnable(GL_DEPTH_TEST);
  oglClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
  //oglTranslatef (0.f, 0.f, -20.f);
  glScalef(cf,cf,cf);
  oglRotatef (time*20.f, 1.f, 0.5f, 0.f);
  kuutio();
}

/** Render something that varies with time and "audio snapshot". */
static
void render_w_shaders(const synti2_synth *s){
  /*  glEnable(GL_DEPTH_TEST);*/

  oglUseProgram(pid);

  render_scene(s);

  /*SDL_GL_SwapBuffers();*/
}
