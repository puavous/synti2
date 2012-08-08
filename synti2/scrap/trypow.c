#include <math.h>
#include <stdio.h>
int main(int argc, char **argv){
  int ii;
  float with_pow[128];
  float no_pow[128];

  for(ii=0; ii<128; ii++){
    with_pow[ii] = 440.0f * powf(2.0f, ((float)ii - 69.0f) / 12.0f );
  }

  /*0.0014 Hz difference at 440:*/
  /*
  no_pow[0]=8.175798915643707f;
  for(ii=1;ii<128;ii++){
    no_pow[ii] = no_pow[ii-1] * 1.0594630943592953f;
  }
  */

  //no_pow[127]=12543.855469f + 0.032;
  no_pow[127]=12543.887469f;
  for(ii=126;ii>=0;ii--){
    no_pow[ii] = no_pow[ii+1] / 1.0594630943592953f;
  }

  /*440 close; below is flat and above sharp*/
  float val = 12543.887469f;
  for(ii=126;ii>=0;ii--){
    no_pow[ii] = (val /= 1.0594630943592953f);
  }


  for(ii=0; ii<128; ii++){
    printf("a=%f b=%f diff=%f \n", with_pow[ii], no_pow[ii], no_pow[ii] - with_pow[ii]);
  }
}
