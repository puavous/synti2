#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <math.h>
#include <signal.h>
#include <string.h>

#include "GL/gl.h"
#include "SDL/SDL.h"

#include <jack/jack.h>
#include <jack/midiport.h>

#include "synti2.h"

typedef jack_default_audio_sample_t sample_t;

jack_client_t *client;
jack_port_t *output_portL;
jack_port_t *output_portR;
jack_port_t *inmidi_port;
unsigned long sr;
char * client_name = "avtest";

synti2_synth *global_synth;
synti2_player *global_player;

int global_hack_playeronly = 0;

synti2_smp_t global_buffer[20000]; /* FIXME: limits? */

extern unsigned char patch_sysex[];  /* Test patch */
extern unsigned char hacksong_data[];  /* Test patch */

/* Snapshot of audiodata, as was hacked for 'Teh 4k 3000': */
#define AUDIOBUFSIZE  4096
GLfloat snapshot[4*AUDIOBUFSIZE];
static long frame = 0;


static void signal_handler(int sig)
{
  jack_client_close(client);
  fprintf(stderr, "signal received, exiting ...\n");
  exit(0);
}


static void
process_audio (jack_nframes_t nframes) 
{
  int i;
  sample_t *bufferL = (sample_t*)jack_port_get_buffer(output_portL, nframes);
  sample_t *bufferR = (sample_t*)jack_port_get_buffer(output_portR, nframes);

  if (global_hack_playeronly == 0){
    synti2_read_jack_midi(global_synth, inmidi_port, nframes);
  }
  synti2_render(global_synth, global_buffer, nframes); 

  for (i=0;i<nframes;i++){
    bufferL[i] = global_buffer[i];
    bufferR[i] = global_buffer[i];
  }
 
  /* FIXME: (Will do this time :)) I ended up with this very ugly way
     to get things happening on screen. Need to expose more of the
     synth state in future versions. Happening right here and now...*/
  frame += nframes;
  if ((global_buffer[5]>0.5)){
    for (i=0;i<AUDIOBUFSIZE;i++) snapshot[i]=global_buffer[i];
  }
}

static int
process (jack_nframes_t nframes, void *arg)
{
  /* FIXME: Should have our own code here, arg could be our data?*/
  process_audio (nframes);
  return 0;
}

static int
init_jack(){ return 0; /*TODO: proper coding practices and all that..*/ }

int
main (int argc, char *argv[])
{
  float tnow;
  SDL_Event event;
  jack_status_t status;

  /* Do some SDL init stuff.. */
  SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER);
  //  SDL_SetVideoMode(640,400,32,SDL_OPENGL);
  SDL_SetVideoMode(1024,768,32,SDL_OPENGL);

  if ((argc >= 2) && (strcmp(argv[1],"-p")==0)){
    global_hack_playeronly = 1;
  }

  /* Initial Jack setup. Open (=create?) a client. */
  if ((client = jack_client_open (client_name, 
                                  JackNoStartServer, 
                                  &status)) == 0) {
    fprintf (stderr, "jack server not running?\n");
    return 1;
  }

  /* Set up process callback */
  jack_set_process_callback (client, process, 0);
  
  output_portL = jack_port_register (client, 
                                     "bportL", 
                                     JACK_DEFAULT_AUDIO_TYPE, 
                                     JackPortIsOutput, 
                                     0);

  output_portR = jack_port_register (client, 
                                     "bportR", 
                                     JACK_DEFAULT_AUDIO_TYPE, 
                                     JackPortIsOutput, 
                                     0);

  inmidi_port = jack_port_register (client, 
                                    "iportti", 
                                    JACK_DEFAULT_MIDI_TYPE, 
                                    JackPortIsInput, 
                                    0);

  sr = jack_get_sample_rate (client);

  /* My own soft synth to be created. */
  if (global_hack_playeronly) {
    global_synth = synti2_create(sr, patch_sysex, hacksong_data);
  } else {
    global_synth = synti2_create(sr, patch_sysex, NULL);
  }

  if (global_synth == NULL){
    fprintf (stderr, "Couldn't allocate synti-kaksi \n");
    goto error;
  };
  

  /* Now we activate our new client. */
  if (jack_activate (client)) {
    fprintf (stderr, "cannot activate client\n");
    goto error;
  }
  
  /* install a signal handler to properly quit jack client */
#ifdef WIN32
  signal(SIGINT, signal_handler);
  signal(SIGABRT, signal_handler);
  signal(SIGTERM, signal_handler);
#else
  signal(SIGQUIT, signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGHUP, signal_handler);
  signal(SIGINT, signal_handler);
#endif

  /* run until interrupted */

  do
  {
    tnow = ((float)frame) / sr;
    render_using_synti2(global_synth);
    SDL_PollEvent(&event);
    usleep(1000000/50); /*50 Hz refresh enough for testing..*/
  } while (event.type != SDL_QUIT); //while (event.type!=SDL_KEYDOWN && tnow <70.0);
  
  jack_client_close(client);
  
 error:
  exit (0);
}
