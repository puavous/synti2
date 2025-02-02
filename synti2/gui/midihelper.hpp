#ifndef MIDIHELPER_H_INCLUDED
#define MIDIHELPER_H_INCLUDED

#include <vector>

/* Functions - maybe belonging to some class? */

namespace synti2{
  float decode_f(unsigned int encoded_fval);
  unsigned int encode_f(float val);
}

void
push_to_sysex_int7b4(std::vector<unsigned char> &v, int intval);

void
push_to_sysex_f(std::vector<unsigned char> &v, float fval);

/** 
 * A common helper function - Encodes an integer value into a MIDI
 * varlength bytestream; returns number of bytes in the result.
 */
int
encode_varlength(unsigned int value, unsigned char *dest);

/**
 * A common helper function - Decodes an integer value from a
 * varlength bytestream; returns the number of bytes read.
 */
int
decode_varlength(const unsigned char *source, unsigned int *dest);

/** 
 * A common helper function - Splits an integer (assumed to be at most
 * 28 nonzero bits) to four 7-bit parts. MSB will be in position 0,
 * and the rest will be in decreasing order of significance. There
 * must be at least 4 bytes of space in the destination buffer.
 *
 * The intended use is the real time/compose mode transmission of data
 * as MIDI SysEx where the bulk data needs to be at most 7 bits wide.
 */
void
encode_split7b4(unsigned int value, unsigned char * dest);

/** Writes the header part of synti2 Sysex messages. */
void
synti2_sysex_header(std::vector<unsigned char> &v);

/** Writes the footer part of synti2 Sysex messages. */
void
synti2_sysex_footer(std::vector<unsigned char> &v);

#endif
