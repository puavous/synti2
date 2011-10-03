/** MISSS - Midi-like interface for Synti2 Software Synthesizer.
 *
 * This program reads and analyzes a standard MIDI file (type 0 or
 * type 1), tries to mutilate it in various ways to reduce its size,
 * and outputs a file compatible with the super-simplistic sequencer
 * part of the Synti2 Software Synthesizer.
 *
 * FIXME: Now I just need to implement this.
 *
 * Ideas:
 *
 * - Trash all events that Synti2 doesn't handle.
 *
 * - Use simplistic note-off (not going to use note-off velocity)
 *
 * - Make exactly one "track" - call it "layer"! - per one MIDI status
 *   word, use the channel nibble of the status byte for something
 *   more useful (which is? Could be "chord size", i.e., number of
 *   instantaneous notes, no need for zero delta!), and write
 *   everything as "running status". Make a very simple layering
 *   algorithm to piece it together upon sequencer creation. Just "for
 *   layer in layers do song.addlayer(layer)" and then start
 *   playing. Will be very simple at that point.
 *
 * - Trash header information that Synti2 doesn't need.
 *
 * - Only one BPM value allowed throughout the song; no tempo map,
 *   sry. (No need to compute much during the playback).
 *
 * - Use lower parts-per-quarter setting, and quantize notes as
 *   needed. The variable-length delta idea is useful as it is.
 *
 * - Decimate velocities
 *
 * - Approximate controllers with linear ramps.
 *
 * - Turn percussion tracks to binary beat patterns.
 *
 */

int main(int argc, char *argc[]){
  return 0;
}
