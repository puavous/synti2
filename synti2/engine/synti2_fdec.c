/** Decode a "floating point" parameter from the special bit pattern
    format. Used in off-line storage (generated by the tool programs);
    not compatible with MIDI SysEx.

  This code (and algorithm) is now localized here; it must match the
  encoding code in synti2_fenc.c

  Format:
  - Input is an integer (actually a MIDI "varlength" of 1-4 bytes, 
    but we assume it is read into an int before calling this function)
  - two least significant bits are the accuracy, as negative power of ten.
  - third least significant bit is the sign (1 = positive, 0 = negative)
  - rest of the bits (at most 25) are the significant digits of the value
  - Output is a single-precision float.

  - examples of one-byte parameters: 0.001 0.015 0.15 -12
  - examples of two-byte parameters: 0.016 -0.21 19 100

 */

float synti2_decode_f(int stored){
  float res;
  res = (stored >> 3);             /* signif. digits */
  res = ((stored & 0x4)?res:-res); /* sign bit */
  for (; (stored & 0x3) > 0; stored--) res /= 10.f; /* neg10pow */
  return res;
}
