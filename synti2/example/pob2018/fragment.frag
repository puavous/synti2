#version 120
uniform float s[200]; // State parameters from app.

vec2 rot(vec2 p, float th){
  float si = sin(th);
  float co = cos(th);
  return vec2(co*p.x - si*p.y, si*p.x + co * p.y);
}


/** 
 * Mandelbrot, with smooth iter count. This is the example from
 * https://www.iquilezles.org/www/articles/mset_smooth/mset_smooth.htm
 */
float mandel( in vec2 c )
{
    const float B = 256.0;

    float n = 0.0;
    vec2 z  = vec2(0.0);
    for( int i=0; i<200; i++ )
    {
        z = vec2( z.x*z.x - z.y*z.y, 2.0*z.x*z.y ) + c; // z = z^2 + c
        if( dot(z,z)>(B*B) ) break;
        n += 1.0;
    }

    //float sn = n - log(log(length(z))/log(B))/log(2.0); // smooth iteration count
    float sn = n - log2(log2(dot(z,z))) + 4.0;  // equivalent optimized smooth iteration count

    return sn;
}

/** Julia set, return smooth iteration count and a trap distance */
vec2 julia(vec2 c, vec2 z, int iter, int deg, vec2 trappoint){
  const float B = 4.0;
  float dist = 1e6;
  int i;
  for(i=0; i<iter; i++) {
      // Evaluate z^deg
      vec2 p = z;
      for(int j=1;j<deg;j++){
          p = vec2(p.x * z.x - p.y * z.y,
                   p.x * z.y + p.y * z.x);
          //p = p.xx*z.xy + vec2(-1.,1.)*p.yy*z.yx;
      }
      // End up with z^deg + c
      z = p + c;
      dist = min(dist,length(z-trappoint));
      if (dot(z,z) > B*B) break;
  }

  float sn = float(i) - log(log(length(z))/log(B))/log(deg); // smooth iteration count

  //float sn = float(i-1) - log2(log2(dot(z,z))) + 4.0;  // equivalent optimized smooth iteration count
  return vec2(sn,dist);

    //return ((i == iter) ? 0.0 : float(i)) / iter;
}

void main(){
    // Cartesian, with aspect ratio:
    vec2 pix = 1. - gl_FragCoord.xy / (.5 * vec2(s[1],s[2]));
    pix.x *= s[1]/s[2];

    pix *= .7;

    // Translate:
    pix = rot(pix,-s[0]/15);

    //pix += .3*vec2(sin(s[0]/9),cos(s[0]/9));


    // Map from screen coordinates to something radially symmetric
    //float l = max(abs(pix.x),abs(pix.y));

    //pix = sin(s[0]/4 + sin(s[0])/(l+.001)) * pix;

    vec2 ctr[5] = vec2[5](
                          vec2(-0.8,0.156),
                          vec2(.4,.6),
                          vec2(-.1104,.123),
                          vec2(.14,-.123),
                          vec2(-.1,.023)
                          );

    //pix=rot(pix,s[0]);

    // Color table
    vec4 ctab[5] = vec4[5](
      vec4(.9, .5, .05, 1.),
      vec4(.2, .6, .1, 1.),
      vec4(1.,0.,1., 1.),
      vec4(s[7],0.,s[9],1.),
      vec4(1.,0.,1.,1.)
    );

    int iter = 18;
    vec2 z0 = pix;
    vec4 col = vec4(0.);
    //vec2 c = vec2(-0.8,0.156); //ctr[3]*4.*sin(s[0]/4.);

    //vec2 c = ctr[1]+sin(s[0]/7)*vec2(sin(s[0]+sin(s[0]/3)),cos(s[0])) + vec2(0,-.1);

    vec2 c = ctr[1]+.04*(vec2(sin(s[0]+sin(s[0]/3)),cos(s[0])) + vec2(0,-1.));

    //vec2 c = ctr[1];

    vec2 tp = rot(pix,s[0])*cos(s[0]*.3); //vec2(.3,.5);
    //vec2 tp = rot(sin(pix*6),s[0])*cos(s[0]*.3); //vec2(.3,.5);
    vec2 jres = julia(c, z0, iter, 3, tp);
    //float j = mandel(z0) / 100;

    col +=  ctab[1] / max(.001,jres.y);

    //col += max(.0,jres.x)/iter * ctab[1];

    //vec2 jres2 = julia(c, .9*z0, iter, 2, tp);
    //col += .2* ctab[1] / max(.001,jres2.y);

    gl_FragColor = col;

  }
