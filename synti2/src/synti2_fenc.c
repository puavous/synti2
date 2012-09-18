/** Encode a "floating point" parameter into 7 bit parts.  Used in
    off-line storage (generated by the tool programs); not compatible
    with MIDI SysEx. See decoder for format description.

  This code (and algorithm) is now localized here; it must match the
  decoding code in synti2_fdec.c

 */
unsigned int synti2_encode_f(float val){
  int negative = 0;
  int neg10 = 0;
  unsigned int intval = 0;
  float tol = 0.0005f;
  float divis = .1f;

  negative = (val < 0.f);
  val = negative?-val:val; /*abs*/

  /* find required accuracy: */
  for (neg10 = 0; neg10 <= 3; neg10++){
    divis *= 10;
    if ((val*divis)-((int)(val*divis)) < tol){
      break;
    }
  }
  /* if it fell through, neg10==4 !!*/
  if (neg10==4) neg10--;

  intval = val*divis;
  intval <<= 2;
  intval += neg10;
  intval <<= 1;
  intval += negative;
  return intval;
}
