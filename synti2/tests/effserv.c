/** Trying out the Instanssi Effect server.. */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define PORT 9909
#define SRV_IP "192.168.10.1"

#define LEFFLEN 6


/* hack for today. */
void init_light_package(unsigned char * buf, int nlights){
  int i;
  memset(buf, 0, nlights*sizeof(unsigned char));
  buf[0]=1; /* Spec version 1 */
  buf++;
  for(i=0;i<nlights;i++){
    buf[i*LEFFLEN] = 1; /* effect type is light. */
    buf[i*LEFFLEN+1] = i; /* light indices. */
  }
}

/* Can set from same values as colors in the OpenGL visualization */
void set_light_rgb_fv(unsigned char * buf, int lightind, float *rgb){
  float f;
  int i;
  for(i=0;i<3;i++){
    /*clamp*/
    if (*rgb < 0.f) f = 0.f;
    else if (*rgb > 1.f) f = 1.f;
    else f = *rgb;
    buf[1 + lightind * LEFFLEN + 3 + i] = (255.f * f);
    rgb++;
  }
}


const char light[] = {
  1, /* Speksin versio aina yksi*/

  1, /* Tehosteen tyyppi on yksi eli valo*/
  0, /* Ensimmäinen valo löytyy indeksistä nolla*/
  0, /* Valon tyyppi on nolla eli RGB*/
  255, /* Punaisuus maksimiin */
  0, /* Vihreys nollaan */
  0, /*Sinisyys nollaan */

  1, /* Toinen tehoste on myöskin valo eli yksi */
    1, /* Toinen valo on indeksissä yksi */
    0, /* Toisen valon tyyppi on myöskin RGB */
/* RGB */
    255,
    0, 
    0,

  1, /* Jne.. */
    2, 
    0, 

    0, 
    0, 
    255, 
};

#define NLIGHTS 4
char light_packet[1+LEFFLEN*NLIGHTS];


void diep(char *s)
{
#ifndef ULTRASMALL
  perror(s);
#endif
  exit(1);
}

int main(void)
{
  struct sockaddr_in si_other;
  int s, i, slen=sizeof(si_other);

  if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1){
    diep("socket");
  }

  /* Zero it out... */
  memset((char *) &si_other, 0, sizeof(si_other));

  si_other.sin_family = AF_INET;
  si_other.sin_port = htons(PORT);

  if (inet_aton(SRV_IP, &si_other.sin_addr)==0) {
#ifndef ULTRASMALL
    fprintf(stderr, "inet_aton() failed\n");
#endif
    exit(1);
  }

  float my_lights[] = {1.f, 0.f, 0.f};
  init_light_package(light_packet, NLIGHTS);

  for(i=0; i<NLIGHTS; i++){
    set_light_rgb_fv(light_packet, i, my_lights);
  }

  int buflen = sizeof(light_packet);

  if (sendto(s, light_packet, buflen, 0, &si_other, slen)==-1){
#ifndef ULTRASMALL
    diep("sendto()");
#endif
  }

  close(s);
  return 0;
}
