/** Encode a "floating point" parameter into 7 bit parts. 

  This code (and algorithm) is now localized here; it must match the
  decoding code in synti2_fdec.c

 */
float synti2_encode_f(float val, unsigned char * buf){
  int high = 0;
  int low = 0;
  int intval = 0;
  int timestimes10 = 0;
  if (val < 0){ high |= 0x40; val = -val; } /* handle sign bit */
  /* maximum precision strategy (?): */
  /* TODO: check decimals first, and try less precise if possible */
  if (val <= 0.511) {
    timestimes10 = 0; intval = val * 1000;
  } else if (val <= 5.11) {
    timestimes10 = 1; intval = val * 100;
  } else if (val <= 51.1) {
    timestimes10 = 2; intval = val * 10;
  } else if (val <= 511) {
    timestimes10 = 3; intval = val * 1;
  } else if (val <= 5110.f) {
    timestimes10 = 4; intval = val * .1;
  } else if (val <= 51100.f) {
    timestimes10 = 5; intval = val * .01;
  } else if (val <= 511000.f) {
    timestimes10 = 6; intval = val * .001;
  } else if (val <= 5110000.f){
    timestimes10 = 7; intval = val * .0001;
  } else {
    timestimes10 = 1; intval = 0;
    /*jack_error("Too large f value %f", val);*/
  }
  high |= (timestimes10 << 2); /* The powers of 10*/
  high |= (intval >> 7);
  low = intval & 0x7f;
  buf[0] = high;
  buf[1] = low;
  // jack_info("%02x %02x", high, low);
  return synti2_decode_f(buf);
}
