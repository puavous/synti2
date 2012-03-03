#include "synti2.h"
#include "synti2_jack.h"
#include "synti2_guts.h"

#include <string.h>

/* The whole thing is relevant only if compiling for Jack. */
#ifdef JACK_MIDI

/** Creates a future for the player object that will repeat the next
 *  nframes of midi data from a jack audio connection kit midi port.
 *
 * ... If need be.. this could be made into merging function on top of
 * the sequence being played.. but I won't bother for today's
 * purposes...
 */
static
void
synti2_player_init_from_jack_midi(synti2_player *pl,
                                  jack_port_t *inmidi_port,
                                  jack_nframes_t nframes)
{
  jack_midi_event_t ev;
  jack_nframes_t i, nev;
  byte_t *msg;
  void *midi_in_buffer  = (void *) jack_port_get_buffer (inmidi_port, nframes);

  /* Re-initialize, and overwrite any former data. */
  pl->playloc = pl->evpool; /* This would "rewind" the song */
  pl->insloc = pl->evpool; /* This would "rewind" the song */
  pl->freeloc = pl->evpool+1;
  pl->idata = 0;
  pl->playloc->next = NULL; /* This effects the deletion of previous msgs.*/
  
  nev = jack_midi_get_event_count(midi_in_buffer);
  for (i=0;i<nev;i++){
    if (jack_midi_event_get (&ev, midi_in_buffer, i) != ENODATA) {

      /* Get next available spot from the data pool */
      msg = pl->data + pl->idata;
      memcpy(msg,ev.buffer,ev.size);  /* deep copy here */
      pl->idata += ev.size; /*Update the data pool top*/

      synti2_player_event_add(pl, 
                              pl->frames_done + ev.time, 
                              msg, 
                              ev.size);
    } else {
      break;
    }
  }
}


/** Interface. */
void
synti2_read_jack_midi(synti2_synth *s,
                      jack_port_t *inmidi_port,
                      jack_nframes_t nframes)
{
  synti2_player_init_from_jack_midi(s->pl, inmidi_port, nframes);
}

#endif
