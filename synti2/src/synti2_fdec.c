/** Decode a "floating point" parameter. 


  This code (and algorithm) is now localized here; it must match the
  encoding code in synti2_fenc.c

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
