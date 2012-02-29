/** Classes for processing MIDI and MISSS sequences and events, either
 * in real-time or off-line.
 *
 */

#include "miditool.hpp"

#include <iostream>

void 
MidiEventTranslator::hack_defaults()
{
  int hack_drummap[12] = {
    /*Chan 9  Oct -5 */ 0,0,1,1,1,3,2,3,2,3,4,3,
  };

  /* For example: 5-poly piano, 2-poly high-string, mono bass, mono lead,
     drums (bd,sd,oh,ch,to), effects? */
  reset_state(); /* Start with nothing, and only set some non-zeros */
  voice_rotate[0]=4; /*Channels 1-4 are 'polyphonic' */
  voice_rotate[5]=2; /*Channels 6-7 are 'biphonic' */

  /* Drum channel diversions as in GM for one octave (bd, sd, sd2 ...): */
  for (int j=0;j<MIDI_NNOTES;j++) {
    channel_table[9][j] = hack_drummap[j % 12];
  }
}

/** Zero everything */
void 
MidiEventTranslator::reset_state(){
  int i,j;
  for(int i=0; i<MIDI_NCHANNELS; i++){
    voice_rotate[i]=1;
    next_rotation[i]=0;
  }
  
  for (i=0;i<MIDI_NCHANNELS;i++){
    /* No notes "playing" when we begin. */
    for (j=0;j<MIDI_NNOTES;j++) rotation_of_noteon[i][j] = -1; 
    for (j=0;j<MIDI_NCHANNELS;j++) note_of_rotation[i][j] = -1;
    /* No notewise diversions ("keyboard splits") */
    for (j=0;j<MIDI_NNOTES;j++) channel_table[i][j] = 0;
  }
}


MidiEventTranslator::MidiEventTranslator(){
  /* FIXME: Zero everything. Maybe make a constructor that can read
   * a file.
   */
  hack_defaults();
}
  

/** Just a dirty hack to rotate channels, if wired to do so.*/
int
MidiEventTranslator::rotate_notes(jack_midi_event_t *ev)
{
  int cmd, chn, note;
  int newrot, oldrot, newchn;

  cmd = ev->buffer[0] >> 4;
  chn = ev->buffer[0] & 0x0f;
  note = ev->buffer[1];

  if (cmd == 0x09){
    /* Note on goes to the next channel in rotation. */
    newrot = next_rotation[chn] % voice_rotate[chn];
    
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
    return 1; /* other messages not handled. */
  }

  ev->buffer[0] = (cmd<<4) + newchn; /* mogrify event. */
  return 1;
}


/** Just a dirty hack to spread drums out to different channels. */
void
MidiEventTranslator::channel(jack_midi_event_t *ev){
  int nib1, nib2, note;
  nib1 = ev->buffer[0] >> 4;
  nib2 = ev->buffer[0] & 0x0f;
  note = ev->buffer[1];

  nib2 = nib2 + channel_table[nib2][note];

  ev->buffer[0] = (nib1<<4) + nib2;
}
