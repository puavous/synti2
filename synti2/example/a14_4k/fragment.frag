#version 120
uniform float s[200]; // State parameters from app.

vec2 rot(vec2 p, float th){
  float si = sin(th);
  float co = cos(th);
  return vec2(co*p.x - si*p.y, si*p.x + co * p.y);
}


float julia(vec2 c, vec2 z, int iter){

  int i;
  for(i=0; i<iter; i++) {
        float x = (z.x * z.x - z.y * z.y) + c.x;
        float y = (z.y * z.x + z.x * z.y) + c.y;

        if((x * x + y * y) > 76.0) break;
        z.x = x;
        z.y = y;
  }

  return ((i == iter) ? 0.0 : float(i)) / 100.0;
}

  void main(){
    vec2 pix = 1. - gl_FragCoord.xy / (.5 * vec2(s[1],s[2]));
    pix.x *= s[1]/s[2];
    pix /= 2.-sin(s[0]);

    int iter = 30;


    vec2 cbase = vec2(.4,.456);
    vec2 c = cbase + vec2(.01*sin(s[0]),.01*cos(s[0]));
    float rotm = sin(s[0]);

    vec4 ctab[5] = vec4[5](
      vec4(1.,0.,0.,1.),
      vec4(0.,1.,0.,1.),
      vec4(0.,0.,1.,1.),
      vec4(1.,1.,0.,1.),
      vec4(0.,1.,1.,1.)
    );

    float th[5] = float[5](s[0],s[0]+rotm*2.,-s[0]*3.,rotm*4.,s[0]);

    vec2 z0=pix;

    vec4 col = vec4(0.);

    int l;
    int lmax = 5;
    for (l=0;l<lmax;l++){
      vec2 z = rot(z0,th[l]);
      float j = julia(c,z * float(l),iter);
      col += j * 5.*ctab[l];
      c = rot(c,.01*s[0]);
      c += vec2(.02*float(l));
    }

    gl_FragColor = col;

  }
