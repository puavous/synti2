#include "midihelper.hpp"

/* A hack (?) to have localized code that is useable as a static
   function inside the core synth. Maybe there is a better way for
   this... need to think.. later.*/
#include "synti2_fdec.c"
#include "synti2_fenc.c"

/** Decode a "floating point" parameter from off-line storage. */
float synti2::decode_f(unsigned int encoded_fval){
  return synti2_decode_f(encoded_fval);
}

/** Encode a "floating point" parameter into 7 bit parts
    ("varlength"). */
unsigned int synti2::encode_f(float val){
  return synti2_encode_f(val);
}

/* 28 bit integer, split into 4 slots of 7 bits. Used in internal
 * compose-mode sysexes.
 */
void
encode_split7b4(unsigned int value, unsigned char * dest){
  dest[0] = (value >> 21) & 0x7f;
  dest[1] = (value >> 14) & 0x7f;
  dest[2] = (value >> 7) & 0x7f;
  dest[3] = (value >> 0) & 0x7f;
}

void
push_to_sysex_int7b4(std::vector<unsigned char> &v, int intval){
    unsigned char buf[4];
    encode_split7b4(intval, buf);
    for(int i=0;i<4;i++){
      v.push_back(buf[i]);
    }
}

void
push_to_sysex_f(std::vector<unsigned char> &v, float fval){
    int intval = synti2::encode_f(fval);
    push_to_sysex_int7b4(v, intval);
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

