#include "midihelper.hpp"

/* For simplicity, use the varlength of SMF.*/
int
encode_varlength(unsigned int value, unsigned char *dest){
  unsigned char bytes[4];
  int i, vllen;
  /* Chop to 7 bit pieces, MSB in position 0: */
  bytes[0] = (value >> 21) & 0x7f;
  bytes[1] = (value >> 14) & 0x7f;
  bytes[2] = (value >> 7) & 0x7f;
  bytes[3] = (value >> 0) & 0x7f;
  /* Set the continuation bits where needed: */
  vllen = 0;
  for(i=0; i<=3; i++){
    if ((vllen > 0) || (bytes[i] != 0) || (i==3)){
      vllen += 1;
      if (i<3) bytes[i] |= 0x80; /* set cont. bit */ 
    }
  }
  /* Put to output buffer MSB first */
  for(i=4-vllen; i<4; i++){
    *(dest++) = bytes[i];
  }
  return vllen; /* return length of encoded byte stream */
}
