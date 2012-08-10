#ifndef MIDIHELPER_H_INCLUDED
#define MIDIHELPER_H_INCLUDED

/** A common helper function. Encodes an integer value into a MIDI
 *  varlength bytestream; returns number of bytes in the result.
 */
int
encode_varlength(unsigned int value, unsigned char *dest);


#endif
