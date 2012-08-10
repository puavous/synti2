/** Decode a "floating point" parameter. 


  This code (and algorithm) is now localized here; it must match the
  encoding code in synti2_fenc.c

  Format:
  - Input is a (varlength, but read beforehand) integer (1-4 bytes)
  - least significant bit is the sign (0 = positive, 1 = negative)
  - shift right to rid of one bit.
  - then, two least significant bits are the accuracy, as negative power of ten.
  - shift right to rid of two bits.
  - the rest of bits are now the value where accuracy and sign are applied.
  - examples of one-byte parameters: 0.001 0.015 0.15 -12
  - examples of two-byte parameters: 0.016 -0.21 19

 */

float synti2_decode_f(unsigned int stored){
  int i;
  float res;
  int negative = 0;
  negative = stored & 0x1; stored >>= 1;
  i = stored & 0x03; stored >>= 2;
  res = stored;
  if (negative) res = -res;
  for (; i > 0; i--) res /= 10.f;
  return res;
}
