/* A jack midi utility to make midi transformations on the fly.
 *
 * For experimenting with the realtime synth while it is being
 * developed.
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <math.h>
#include <signal.h>
#include <string.h>

#include <jack/jack.h>
#include <jack/midiport.h>

typedef jack_default_audio_sample_t sample_t;

jack_client_t *client;
jack_port_t *inmidi_port;
jack_port_t *outmidi_port;
unsigned long sr;
char * client_name = "jmiditrans";

/* For example: 5-poly piano, 2-poly high-string, mono bass, mono lead,
   drums (bd,sd,oh,ch,to), effects? */
/* FIXME: Wrap all this to a class with load/save/edit option and a GUI.*/
int voice_rotate[16] = {4,0,0,0,
                        0,1,0,0,
                        0,0,0,0,
                        0,0,0,0};

int next_rotation[16] = {0,0,0,0,
                         0,0,0,0,
                         0,0,0,0,
                         0,0,0,0};

int note_of_rotation[16][16];

int rotation_of_noteon[16][128]; /* FIXME: to -1*/


int last_noteon[16] ={-1,-1,-1,-1,
                      -1,-1,-1,-1,
                      -1,-1,-1,-1,
                      -1,-1,-1,-1};

int channel_table[16][128] = {
  /*Chan 0  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan 1  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan 2  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan 3  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan 4  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan 5  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan 6  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan 7  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan 8  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan 9  Oct -5 */ {0,0,1,1,1,3,2,3,2,3,4,3,
  /*        Oct -4 */  0,0,1,1,1,3,2,3,2,3,4,3,
  /*        Oct -3 */  0,0,1,1,1,3,2,3,2,3,4,3,
  /*        Oct -2 */  0,0,1,1,1,3,2,3,2,3,4,3,
  /*        Oct -1 */  0,0,1,1,1,3,2,3,2,3,4,3,
  /*        Oct  0 */  0,0,1,1,1,3,2,3,2,3,4,3,
  /*        Oct  1 */  0,0,1,1,1,3,2,3,2,3,4,3,
  /*        Oct  2 */  0,0,1,1,1,3,2,3,2,3,4,3,
  /*        Oct  3 */  0,0,1,1,1,3,2,3,2,3,4,3,
  /*        Oct  4 */  0,0,1,1,1,3,2,3,2,3,4,3,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan a  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan b  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan c  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan d  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan e  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan f  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},
};
//synti2_smp_t global_buffer[20000]; /* FIXME: limits? */

static void signal_handler(int sig)
{
  jack_client_close(client);
  fprintf(stderr, "stop\n");
  exit(0);
}


static void
debug_print_ev(jack_midi_event_t *ev){
  jack_info("Msg (len %d) at time %d:", ev->size, ev->time);
  if (ev->size==3) jack_info("  3byte: %02x %02x %02x", ev->buffer[0],
                             ev->buffer[1], ev->buffer[2]);
}


/** Just a dirty hack to rotate channels, if wired to do so.*/
static 
int
rotate_notes(jack_midi_event_t *ev){
  int cmd, chn, note;
  int newrot, oldrot, newchn;

  cmd = ev->buffer[0] >> 4;
  chn = ev->buffer[0] & 0x0f;
  note = ev->buffer[1];

  if (cmd == 0x09){
    /* Note on goes to the next channel in rotation. */
    if (voice_rotate[chn] > 0) {
      newrot = next_rotation[chn] % voice_rotate[chn];
    } else {
      newrot = 0;
    }
    next_rotation[chn]++; /* tentative for next note-on. */
    newchn = chn + newrot;  /* divert to the rotated channel */

    /* Keep a record of where note ons have been put. When there is an
     * overriding note-on, earlier note-on must be erased, so that
     * note-off can be skipped (the note is "lost"/superseded)
     */
    if (note_of_rotation[chn][newrot] >= 0){
      /* Forget the superseded note-on: */
      rotation_of_noteon[chn][note_of_rotation[chn][newrot]] = -1;
    }
    /* remember the new noteon */
    note_of_rotation[chn][newrot] = note;
    rotation_of_noteon[chn][note] = newrot;
  } else if (cmd == 0x08){
    /* Note off goes to the same channel, where the corresponding note
       on was located earlier. FIXME: should swallow if note on is no
       more relevant.  */
    if (rotation_of_noteon[chn][note] < 0) return 0; /* Dismissed already*/

    oldrot = rotation_of_noteon[chn][note];
    note_of_rotation[chn][oldrot] = -1; /* no more note here. */
    newchn = chn+oldrot; /* divert note-off to old target. */
  } else {
    return 1;
  }

  ev->buffer[0] = (cmd<<4) + newchn; /* mogrify event. */
  return 1;
}


/** Just a dirty hack to spread drums out to different channels. */
static void
channel(jack_midi_event_t *ev){
  int nib1, nib2, note;
  nib1 = ev->buffer[0] >> 4;
  nib2 = ev->buffer[0] & 0x0f;
  note = ev->buffer[1];

  nib2 = nib2+channel_table[nib2][note];

  ev->buffer[0] = (nib1<<4) + nib2;
}

static int
solo_notes(const jack_midi_event_t *ev){
  int cmd, chn, note, vel;
  cmd = ev->buffer[0] >> 4;
  chn = ev->buffer[0] & 0x0f;
  note = ev->buffer[1];

  if (cmd==0x9) {
    //vel = ev->buffer[2];
    last_noteon[chn] = note;
    return 1; /* register and accept. */
  }

  if (cmd==0x8) {
    return (last_noteon[chn] == note); /* accept only if off goes
                                               the same note that was
                                               last on.*/
  }
  return 1; /* Accept all other commands with no problemos. */
}


/** MIDI filtering is the only thing this client is required to do. */
static void
process_audio (jack_nframes_t nframes) 
{
  jack_midi_event_t ev;
  jack_nframes_t i, nev;
  jack_midi_data_t *msg;

  jack_midi_data_t *outdata;
  /*
  outdata = jack_midi_event_reserve(void *midi_out_buffer,
		jack_nframes_t  	time,
		size_t  	data_size 
	) 	
  */
  void *midi_in_buffer  = (void *) jack_port_get_buffer (inmidi_port, nframes);
  void *midi_out_buffer  = (void *) jack_port_get_buffer (outmidi_port, nframes);


  jack_midi_clear_buffer(midi_out_buffer); 
  nev = jack_midi_get_event_count(midi_in_buffer);
  for (i=0;i<nev;i++){
    if (jack_midi_event_get (&ev, midi_in_buffer, i) == ENODATA) break;

    /*debug_print_ev(&ev);*/
    channel(&ev);
    /*debug_print_ev(&ev);*/
    
    if (!rotate_notes(&ev)) continue;
    /* TODO: duplicate_controllers() -- 
       requires creation of additional events!!!
       maybe put the event writing inside of the functions 
       (and refactor names)
     */

    jack_midi_event_write(midi_out_buffer, 
                          ev.time, ev.buffer, ev.size);
  }
}

static int
process (jack_nframes_t nframes, void *arg)
{
  /* FIXME: Should have our own code here, arg could be our data?*/
  process_audio (nframes);
  return 0;
}

int
main (int argc, char *argv[])
{
  int i,j;
  jack_status_t status;
  
  /* No notes "playing" when we begin. */
  for (i=0;i<16;i++){
    for (j=0;j<128;j++) rotation_of_noteon[i][j] = -1; 
    for (j=0;j<16;j++) note_of_rotation[i][j] = -1;
  }

  /* Initial Jack setup. Open (=create?) a client. */
  if ((client = jack_client_open (client_name, JackNoStartServer, &status)) == 0) {
    fprintf (stderr, "jack server not running?\n");
    return 1;
  }
  /* Set up process callback */
  jack_set_process_callback (client, process, 0);
  

  /* Try to create a midi port */
  inmidi_port = jack_port_register (client, "iportti", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  outmidi_port = jack_port_register (client, "oportti", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

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
  while (1) {
#ifdef WIN32
    Sleep(1000);
#else
    sleep(1);
#endif
  };
  
  jack_client_close(client);
  
 error:
  exit (0);
}
