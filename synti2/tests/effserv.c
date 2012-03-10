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

  int buflen = sizeof(light);

  if (sendto(s, light, buflen, 0, &si_other, slen)==-1){
#ifndef ULTRASMALL
    diep("sendto()");
#endif
  }

  close(s);
  return 0;
}
