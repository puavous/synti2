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

    // Map from screen coordinates to something radially symmetric

    //pix /= .5 + s[0]/10.;
    //float l = length(pix);
    float l = max(abs(pix.x),abs(pix.y));
    pix = sin(s[0]/(l+.001)) * pix;

    int iter = 30;

    //vec2 cbase = vec2(.1104,.023);
    //vec2 cbt = vec2(.1104,.023);
    //vec2 c = cbase + (1.+s[0])*.1*vec2(sin(s[0]),cos(s[0]));
    vec2 ctr[5] = vec2[5](
                          vec2(.1104,.023),
                          vec2(.104,-.023),
                          vec2(-.1104,.123),
                          vec2(.14,-.123),
                          vec2(-.1,.023)
                          );
    
    float rotm = s[0];

    vec4 ctab[5] = vec4[5](
      vec4(1.,0.,0.,1.),
      vec4(0.,0.,1.,1.),
      vec4(1.,0.,1.,1.),
      vec4(0.,0.,2.,1.),
      vec4(1.,0.,1.,1.)
    );

    vec2 ztab[5] = vec2[5](
      vec2(1.,0.),
      vec2(0.,1.),
      vec2(1.,1.),
      vec2(-1.,-1.),
      vec2(1.,-1.)
    );
    vec2 z0=pix;

    vec4 col = vec4(0.);

    //z0 = rot(z0,s[0]);

    int l;
    int lmax = 5;
    for (l=0;l<lmax;l++){
      vec2 c = ctr[l] + s[0]*.1*vec2(sin(s[0]),cos(s[0]));
      vec2 z = z0 + ztab[l];
      float j = julia(c,z,iter);
      col += j * 5.*ctab[l];
    }

    gl_FragColor = col;

  }
