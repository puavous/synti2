/** Decode a "floating point" parameter. 


  This code (and algorithm) is now localized here; it must match the
  encoding code in synti2_fenc.c

  FIXME: Idea of the final format which I hope will be somewhat more
  compact in size:

  - Read a varlength integer (1-4 bytes)
  - least significant bit is the sign (0 = positive, 1 = negative)
  - shift right to rid of one bit.
  - then, two least significant bits are the accuracy, as in current spec.
  - shift right to rid of two bits.
  - the rest of bits are now the value where accuracy and sign are applied.
  - examples of one-byte parameters: 0.001 0.015 0.15 -12
  - examples of two-byte parameters: 0.016 -0.21 19

  FIXME: Both encoder and decoder must somehow communicate the size to
  caller (to keep track of read/write pointer). The size given by the
  encoder must be optimal; user interface must show the size for
  educated decisions.
 */

float synti2_decode_f(const unsigned char *buf){
  int i;
  float res;
  res = ((buf[0] & 0x03) << 7) + buf[1];   /* 2 + 7 bits accuracy*/
  res = ((buf[0] & 0x40)>0) ? -res : res;  /* sign */
  res *= .001f;                            /* default e-3 */
  for (i=0; i < ((buf[0] & 0x0c)>>2); i++) res *= 10.f;  /* can be more */
  return res;
}
