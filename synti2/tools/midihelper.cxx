#include "midihelper.hpp"

/* The 7 bits by 4 slots split*/
void
encode_split7b4(unsigned int value, unsigned char * dest){
  dest[0] = (value >> 21) & 0x7f;
  dest[1] = (value >> 14) & 0x7f;
  dest[2] = (value >> 7) & 0x7f;
  dest[3] = (value >> 0) & 0x7f;
}

/* For simplicity, use the varlength of SMF.*/
int
encode_varlength(unsigned int value, unsigned char *dest){
  unsigned char bytes[4];
  int i, vllen;
  encode_split7b4(value, bytes);

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

int
decode_varlength(const unsigned char *source, unsigned int *dest)
{
  /* Just a copy-paste from the static function in the synth engine.. */
  int nread;
  unsigned char byte;
  *dest = 0;
  for (nread=1; nread<=4; nread++){
    byte = *source++;
    *dest += (byte & 0x7f);
    if ((byte & 0x80) == 0){
      return nread; 
    }
    else *dest <<= 7;
  }
  /* Longer than 4 bytes! Actually unexpected input which should be
     caught as a bug. */
  return 0;
}


void
synti2_sysex_header(std::vector<unsigned char> &v){
  v.push_back(0xF0); /* Start sysex*/
  v.push_back(0x00); /* Device ID. I go as anonymous0..*/
  v.push_back(0x00); 
  v.push_back(0x00);
}

void
synti2_sysex_footer(std::vector<unsigned char> &v){
  /* TODO: Could have a checksum here..*/
  v.push_back(0xF7); /*End sysex*/
}
