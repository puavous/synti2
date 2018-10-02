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
      vec2 p = vec2((z.x * z.x - z.y * z.y) + c.x,
                    (z.y * z.x + z.x * z.y) + c.y);
      if (length(p) > 2.0) break;
      z = p;
      //        float x = (z.x * z.x - z.y * z.y) + c.x;
      //        float y = (z.y * z.x + z.x * z.y) + c.y;
      //        if((x * x + y * y) > 4.0) break;
      //        z.x = x;
      //        z.y = y;
  }

  return ((i == iter) ? 0.0 : float(i)) / iter;
}

void main(){
    // Cartesian, with aspect ratio:
    vec2 pix = 1. - gl_FragCoord.xy / (.5 * vec2(s[1],s[2]));
    pix.x *= s[1]/s[2];

    // Map from screen coordinates to something radially symmetric
    //float l = max(abs(pix.x),abs(pix.y));

    //pix = sin(s[0]/4 + sin(s[0])/(l+.001)) * pix;

    vec2 ctr[5] = vec2[5](
                          vec2(.1104,.023),
                          vec2(.104,-.023),
                          vec2(-.1104,.123),
                          vec2(.14,-.123),
                          vec2(-.1,.023)
                          );

    pix=rot(pix,s[0]);

    // Color table
    vec4 ctab[5] = vec4[5](
      vec4(1.,.5,0.,1.),
      vec4(0.,0.,1.,1.),
      vec4(1.,0.,1.,1.),
      vec4(s[7],0.,s[9],1.),
      vec4(1.,0.,1.,1.)
    );

    int iter = 17;
    vec2 z0 = pix;
    vec4 col = vec4(0.);
    vec2 c = ctr[3]*4.*sin(s[0]/4.);

    float j = julia(c,z0,iter);
    col += j * 3.*ctab[0];

    //z0 = rot(z0,s[0]);

    gl_FragColor = col;

  }
