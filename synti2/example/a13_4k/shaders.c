/* Some simple ray marching experiments...
 * 
 * Yep. This is mostly copy-paste from Iñigo Quilez's work. Thanx,
 * dude, even if I don't know you (yet) ;).
 *
 * But a note to our students: I couldn't have done this without the
 * basic vector math and stuff taught on this course.
 *
 * I tried these with my soft synth, of course, but these should be
 * applicable in any other code that renders a rectangle [-1,1]^2 and
 * transfers 9 uniform parameters from the application..
 */
//#define NEED_DEBUG

const GLchar *vs="\
  void main(){                                  \
    gl_Position = ftransform();                 \
  }";

#if 0

/* A crude minified version (glslunit.appspot.com/compiler.html) */
const GLchar *fs= "\
uniform float s[9];vec3 u(vec3 e,float f){return vec3(sin(f)*e.x-cos(f)*e.z,e.y,cos(f)*e.x+sin(f)*e.z);}vec3 v(vec3 e,float f){return vec3(e.x,sin(f)*e.y-cos(f)*e.z,cos(f)*e.y+sin(f)*e.z);}float w(vec3 e,vec3 f,float g){vec3 h=abs(e)-f;return min(max(h.x,max(h.y,h.z)),0.)+length(max(h,0.))-g;}float x(vec3 e,vec3 f,float g){vec3 h=u(e,e.y*sin(s[0])*.4+1.+s[5]);h=v(h,h.z*sin(s[0]+1.)*.1);return w(h,f,g);}vec3 y(vec3 e,vec3 f){return mod(e,f);}float z(vec3 e){vec3 f,g;f=vec3(12,12,25);e.z+=10.*s[0];g=y(v(e,.1*s[0]),f)-.5*f;return x(g,vec3(3,3,2.2),.5);}float A(vec3 e){return z(e);}const float a=.08;const float b=.1;const int c=140;const float d=80.;vec4 B(vec3 e,vec3 f){float g=0.;int h;vec3 i;for(h=0;h<c;h++){i=e+g*f;float j=A(i)*.8;g+=j;if(j<a)break;if(g>d)return vec4(i,0);}vec4 j;j.xyz=i;j.w=1.-float(h)/float(c);return j;}vec3 C(vec3 e){return normalize(vec3(A(e+vec3(b,0,0))-A(e-vec3(b,0,0)),A(e+vec3(0,b,0))-A(e-vec3(0,b,0)),A(e+vec3(0,0,b))-A(e-vec3(0,0,b))));}vec4 D(vec3 e,vec3 f,vec3 g,vec3 h,vec3 i,vec3 j,vec3 k,vec3 l){vec3 m,n,q,r,s,t;m=normalize(h-f);n=j;float o,p;o=length(h-f);p=1./(1.+.03*o+.003*o*o);q=normalize(f-e);r=k*max(dot(g,m),0.);s=reflect(m,g);t=l*pow(max(dot(s,q),0.),4.);n+=p*i*(r+t);return vec4(n,1);}void main(){vec3 e,f,h,j,l;e=vec3(0,0,-20.);f=vec3(sin(s[0]),cos(s[0]*.4),cos(s[0]*.1));f*=20.;vec2 g=gl_FragCoord.xy/vec2(512,384)-vec2(1);h=vec3(g.x,g.y,1);vec4 i=B(e,h);j=i.xyz;float k=i.w;l=C(j);if(k>0.){vec3 m,n,o,p;m=vec3(1);n=vec3(.3,0,0);o=vec3(1,.1,.05);p=vec3(1,1,0);vec4 q=D(e,j,l,f,m,n,o,p);float r=1.-max(distance(e,j)/120.,0.);gl_FragColor=3.*q*r;}else gl_FragColor=vec4(0);}";

#endif

#if 1
const GLchar *fs= "\
  uniform float s[9]; // State parameters from app.                     \
                                                                        \
// FIXME: Should do proper rotation matrices?                           \
vec3 rotY(vec3 p, float th){                                            \
  return vec3(sin(th)*p.x-cos(th)*p.z,                                  \
              p.y,                                                      \
              cos(th)*p.x+sin(th)*p.z);                                 \
}                                                                       \
                                                                        \
vec3 rotZ(vec3 p, float th){                                            \
  return vec3(sin(th)*p.x-cos(th)*p.y,                                  \
              cos(th)*p.x+sin(th)*p.y,                                  \
              p.z);                                                     \
}                                                                       \
                                                                        \
vec3 rotX(vec3 p, float th){                                            \
  return vec3(p.x,                                                      \
              sin(th)*p.y-cos(th)*p.z,                                  \
              cos(th)*p.y+sin(th)*p.z);                                 \
}                                                                       \
                                                                        \
vec3 warpXYZ(vec3 p, vec3 amount){                                      \
  vec3 rp = p;                                                          \
  rp = rotX(rp,p.x*amount.x);                                           \
  rp = rotY(rp,p.y*amount.y);                                           \
  rp = rotZ(rp,p.z*amount.z);                                           \
  return rp;                                                            \
}                                                                       \
                                                                        \
vec3 deRep( vec3 p, vec3 c )                                            \
{                                                                       \
    return mod(p,c);                                                    \
}                                                                       \
                                                                        \
float sdTorus( vec3 p, vec2 t )                                         \
{                                                                       \
  vec2 q = vec2(length(p.xy)-t.x,p.z);                                  \
  return length(q)-t.y;                                                 \
}                                                                       \
                                                                        \
// A heart-shape                                                        \
float heart(vec3 p, vec2 wt)                                            \
{                                                                       \
  p.y -= abs(p.x);                                                      \
  return sdTorus(p, vec2(wt.x,wt.y));                                   \
}                                                                       \
                                                                        \
float warpedHeart(vec3 p, vec2 b)                                       \
{                                                                       \
  p = warpXYZ(p, vec3(0.,.1,sin(s[0])));                                \
  return heart(p, b);                                                   \
}                                                                       \
                                                                        \
                                                                        \
float h(vec3 p){                                                        \
  p = rotZ(p,s[0]*.1);                                                  \
  p = rotX(p,s[0]*.2);                                                  \
  p = rotY(p,s[0]);                                                     \
//  return warpedHeart(p, vec2(5.,1.));                                 \
  float f1 = heart(p+vec3(2.,0.,0.), vec2(5.,1.));                      \
  p = rotX(p,2.142+sin(s[0]*.2));                                       \
  float f2 = heart(p, vec2(5.+s[3]*2.,1.));				\
  return min(f1,f2);                                                    \
}                                                                       \
                                                                        \
float g(vec3 p){                                                        \
  p = deRep(p, vec3(20.,20.,20.));                                      \
  p -= vec3(s[0],.5,.5);                                                  \
  return heart(p, vec2(5.,1.));                                         \
}                                                                       \
                                                                        \
float f(vec3 p){                                                        \
  return h(p);                                                          \
}                                                                       \
                                                                        \
                                                                        \
const float MinimumDistance = .01; // FIXME: accuracy vs. frame rate    \
const float epsilon = 0.1;                                              \
const int MaxRaySteps = 180;                                            \
const float TooFar = 80.0;                                              \
                                                                        \
vec4 march(vec3 from, vec3 direction) {                                 \
  float totalDistance = 0.0;						\
  int steps;								\
  vec3 p;                                                               \
  for (steps=0; steps < MaxRaySteps; steps++) {				\
    p = from + totalDistance * direction;				\
    float distance = f(p)*.9;                                           \
    totalDistance += distance;						\
    if (distance < MinimumDistance){					\
      break;                                                            \
    }                                                                   \
    if (totalDistance > TooFar) return vec4(p,0.0);                     \
  }									\
  vec4 res; res.xyz = p;                                                \
  res.w = 1.-float(steps)/float(MaxRaySteps);				\
  return res;								\
}                                                                       \
                                                                        \
// Distance estimation describes a hyperplane (local tangent plane of   \
// a geometry) in an implicit form (f(p)==0 exactly on the plane,       \
// f(p)>0 on the front side). Gradient of the function gives a vector   \
// normal to the plane. Here we approximate the gradient with finite    \
// differences along the coordinate axes.                               \
                                                                        \
vec3 normalEstimation(vec3 p){                                          \
  return normalize( vec3(                                               \
      f(p + vec3(epsilon,0.,0.) ) - f(p - vec3(epsilon,0.,0.))          \
    , f(p + vec3(0.,epsilon,0.) ) - f(p - vec3(0.,epsilon,0.))          \
    , f(p + vec3(0.,0.,epsilon) ) - f(p - vec3(0.,0.,epsilon))          \
) );}                                                                   \
                                                                        \
                                                                        \
// Phong model                                                          \
vec4 doLightPhong(vec3 pcam, vec3 p, vec3 n, vec3 lpos,                 \
                  vec3 lightC, vec3 amb, vec3 dfs, vec3 spec)           \
{                                                                       \
  // Ambient component:                                                 \
  vec3 c = amb;                                                         \
                                                                        \
  // Compute light direction; finished if light is behind the surface:  \
  vec3 ldir = normalize(lpos - p);                                      \
  if (ldir.z > 0.) return vec4(c,1.);                                   \
                                                                        \
  // Diffuse and specular component:                                    \
  float ldist = length(lpos-p);                                         \
  float attn = 1. / (1. + 0.03*ldist + 0.003*ldist*ldist);              \
  vec3 camdir = normalize(p-pcam);                                      \
  vec3 idfs = dfs * max(dot(n,ldir),0.0);                               \
  vec3 refldir = reflect(ldir, n);                                      \
  vec3 ispec = spec * pow(max(dot(refldir,camdir),0.0),4.0);            \
  c += attn * lightC * (idfs + ispec);                                  \
  return vec4 (c,1.); // no alpha blending in use...                    \
}                                                                       \
                                                                        \
  void main(){                                                          \
    vec3 cameraPosition = vec3(0.,0.,-20.);                             \
    vec3 lightPosition = vec3(sin(s[0]),cos(s[0]*.4),-2.);              \
    lightPosition *= 10.;                                               \
                                                                        \
    // I just shoot 'over there'.                                       \
    // TODO: Proper vector length and direction; from resol.            \
    vec2 pix = gl_FragCoord.xy / vec2(s[1]/2.,s[2]/2.) - vec2(1.,1.);   \
    pix.x *= s[1]/s[2];                                                 \
    vec3 vdir = vec3(pix.x,pix.y,1.);                                   \
    // Hmm.. think about how the direction affects the rendering:       \
    vdir = normalize(vdir) * .5;                                        \
                                                                        \
    vec4 pr = march(cameraPosition,vdir);                               \
    vec3 p = pr.xyz;                                                    \
    float r = pr.w;                                                     \
    vec3 n = normalEstimation(p);                                       \
                                                                        \
    // White dot in the center, for debugging:                          \
    // FIXME: Don't leave this in final version, pls.                   \
    if (length(pix) < 0.01){                                            \
      gl_FragColor = vec4(1.0,1.0,1.0,0.5);                             \
      return;                                                           \
    }                                                                   \
    // Lighting computation.                                            \
    if (r>0.0){                                                         \
      vec3 lightC = vec3(1.);                                           \
      vec3 ambient = vec3(0.3,0.0,0.0);                                 \
      vec3 diffuse = vec3(1.0,0.1,0.05);                                \
      vec3 specular = vec3(1.0,1.0,0.0);                                \
                                                                        \
      vec4 color = doLightPhong(cameraPosition, p, n,                   \
                             lightPosition,                             \
                             lightC,ambient,diffuse,specular);          \
                                                                        \
      float darken = 1.0-max((distance(cameraPosition,p)/120.0),0.0);   \
      gl_FragColor = 3.0*color * darken * r;                            \
    } else {                                                            \
//      gl_FragColor = vec4(0.);                                        \
// Could have some action in the background:
      vec3 bgp = rotZ(vec3(pix,0.),.2*s[0]);                            \
      gl_FragColor = vec4(sin(s[0])*cos(4.*bgp.x*bgp.y)+s[3]);               \
    }                                                                   \
  }";

#endif
