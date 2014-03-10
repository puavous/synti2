/* Some simple ray marching experiments...
 *
 * This by-the-book raymarching engine was originally an example
 * program for the course "Tietokonegrafiikan perusteet" (Fundamentals
 * of Computer Graphics) held at the Department of Mathematical
 * Information Technology, University of Jyväskylä,
 * Finland. (Tietotekniikan laitos / Jyväskylän yliopisto) during the
 * Spring period 2013.
 *
 * Yep. This is mostly copy-paste from Iñigo Quilez's work. Thanx,
 * dude, even if I don't know you (yet) ;).
 *
 * But a note to our students: I couldn't have done this without the
 * basic vector math and stuff taught on this course of ours.
 *
 * I tried these with my soft synth, of course, but these should be
 * applicable in any other code that renders a rectangle [-1,1]^2 and
 * transfers 9 uniform parameters from the application.. In a later
 * revision, I'll make the synth state accessible in a much more easy
 * way ...
 *
 * s[0] needs to be time since the beginning of the animation
 * s[1] needs to be screen width
 * s[2] needs to be screen height
 * s[3..] are parameters affecting the animation.
 */
/* #define NEED_DEBUG */

const GLchar *vs="\
  void main(){                                  \
    gl_Position = ftransform();                 \
  }";

#if 1
/* This version is processed using shader_minifier.exe by ctrl-alt-test 
 * http://www.ctrl-alt-test.fr/?page_id=7
 * Cool stuff.. The result after packing is 10 bytes smaller than with 
 * the previous tool.
 */
const GLchar *fs= ""
 "uniform float s[2000];"
 "vec3 v(vec3 v,float y)"
 "{"
   "float r,f;"
   "r=sin(y);"
   "f=cos(y);"
   "return vec3(r*v.x-f*v.z,v.y,f*v.x+r*v.z);"
 "}"
 "vec3 n(vec3 v,float y)"
 "{"
   "float r,f;"
   "r=sin(y);"
   "f=cos(y);"
   "return vec3(r*v.x-f*v.y,f*v.x+r*v.y,v.z);"
 "}"
 "vec3 f(vec3 v,float y)"
 "{"
   "float r,f;"
   "r=sin(y);"
   "f=cos(y);"
   "return vec3(v.x,r*v.y-f*v.z,f*v.y+r*v.z);"
 "}"
 "float e(vec3 v,vec2 y)"
 "{"
   "vec2 r=vec2(length(v.xy)-y.x,v.z);"
   "return length(r)-y.y;"
 "}"
 "float t(vec3 v,vec2 y)"
 "{"
   "return v.y-=abs(v.x),e(v,vec2(y.x,y.y));"
 "}"
 "float e(vec3 y)"
 "{"
   "y=n(y,s[0]*.1);"
   "y=f(y,s[0]*.2);"
   "y=v(y,s[0]);"
   "float r,z;"
   "r=t(y+vec3(4,0,0),vec2(5,1));"
   "y=f(y,3.14+sin(s[12]*.2));"
   "z=t(y,vec2(5.+s[13]*2.,1));"
   "return min(r,z);"
 "}"
 "float f(vec3 y)"
 "{"
   "y=v(y,s[0]);"
   "float r=t(y,vec2(7.+(mod(s[11],11.)*.2),1));"
   "return r;"
 "}"
 "float n(vec3 y)"
 "{"
   "if(s[0]<29.6)"
     "return f(y);"
   "else"
     " return e(y);"
 "}"
 "const float z=.01,y=.1;"
 "const int r=180;"
 "const float i=80.;"
 "vec4 c(vec3 v,vec3 y)"
 "{"
   "float f=0.;"
   "int g;"
   "vec3 c;"
   "for(g=0;g<r;g++)"
     "{"
       "c=v+f*y;"
       "float t=n(c)*.9;"
       "f+=t;"
       "if(t<z)"
         "break;"
       "if(f>i)"
         "return vec4(c,0);"
     "}"
   "vec4 t;"
   "t.xyz=c;"
   "t.w=1.-float(g)/float(r);"
   "return t;"
 "}"
 "vec3 c(vec3 v)"
 "{"
   "return normalize(vec3(n(v+vec3(y,0,0))-n(v-vec3(y,0,0)),n(v+vec3(0,y,0))-n(v-vec3(0,y,0)),n(v+vec3(0,0,y))-n(v-vec3(0,0,y))));"
 "}"
 "vec4 c(vec3 y,vec3 v,vec3 r,vec3 f,vec3 x,vec3 t,vec3 c,vec3 n)"
 "{"
   "vec3 g,z,i,d,e,w;"
   "g=t;"
   "z=normalize(f-v);"
   "if(z.z>0.)"
     "return vec4(g,1);"
   "float m,l;"
   "m=length(f-v);"
   "l=1./(1.+.03*m+.003*m*m);"
   "i=normalize(v-y);"
   "d=c*max(dot(r,z),0.);"
   "e=reflect(z,r);"
   "w=n*pow(max(dot(e,i),0.),4.);"
   "g+=l*x*(d+w);"
   "return vec4(g,1);"
 "}"
 "void main()"
 "{"
   "vec3 v,y,r,f,z;"
   "v=vec3(0,0,-20.);"
   "y=vec3(-10.);"
   "vec2 g=gl_FragCoord.xy/vec2(s[1]/2.,s[2]/2.)-vec2(1);"
   "g.x*=s[1]/s[2];"
   "r=vec3(g.x,g.y,1);"
   "r=normalize(r)*.5;"
   "vec4 t=c(v,r);"
   "f=t.xyz;"
   "float m=t.w;"
   "z=c(f);"
   "if(m>0.)"
     "{"
       "vec3 i,e,x,w;"
       "i=vec3(3);"
       "e=vec3(.3,0,0);"
       "x=vec3(1,0,0);"
       "w=vec3(1,1,0);"
       "vec4 l=c(v,f,z,y,i,e,x,w);"
       "float d=1.-max(distance(v,f)/120.,0.);"
       "gl_FragColor=l*d*m;"
     "}"
   "else"
     " if(s[0]>15.)"
       "{"
         "vec3 i=n(vec3(g,0),.2*s[0]);"
         "gl_FragColor=vec4(sin(s[0])*cos(4.*i.x*i.y)+s[12]);"
       "}"
     "else"
       " gl_FragColor=vec4(0);"
   "gl_FragColor*=1.-smoothstep(56.,65.,s[0]);"
 "}";
#endif

#if 0
"\
uniform float s[9];vec3 u(vec3 e,float f){float g,h;g=sin(f);h=cos(f);return vec3(g*e.x-h*e.z,e.y,h*e.x+g*e.z);}vec3 v(vec3 e,float f){float g,h;g=sin(f);h=cos(f);return vec3(g*e.x-h*e.y,h*e.x+g*e.y,e.z);}vec3 w(vec3 e,float f){float g,h;g=sin(f);h=cos(f);return vec3(e.x,g*e.y-h*e.z,h*e.y+g*e.z);}float x(vec3 e,vec2 f){vec2 g=vec2(length(e.xy)-f.x,e.z);return length(g)-f.y;}float y(vec3 e,vec2 f){e.y-=abs(e.x);return x(e,vec2(f.x,f.y));}float z(vec3 e){e=v(e,s[0]*.1);e=w(e,s[0]*.2);e=u(e,s[0]);float f,g;f=y(e+vec3(4,0,0),vec2(5,1));e=w(e,3.14+sin(s[0]*.2));g=y(e,vec2(5.+s[6]*2.,1));return min(f,g);}float A(vec3 e){e=u(e,s[0]);float f=y(e,vec2(7.+s[6]*2.,1));return f;}float B(vec3 e){if(s[0]<29.6)return A(e);else return z(e);}const float a=.01;const float b=.1;const int c=180;const float d=80.;vec4 C(vec3 e,vec3 f){float g=0.;int h;vec3 i;for(h=0;h<c;h++){i=e+g*f;float j=B(i)*.9;g+=j;if(j<a)break;if(g>d)return vec4(i,0);}vec4 j;j.xyz=i;j.w=1.-float(h)/float(c);return j;}vec3 D(vec3 e){return normalize(vec3(B(e+vec3(b,0,0))-B(e-vec3(b,0,0)),B(e+vec3(0,b,0))-B(e-vec3(0,b,0)),B(e+vec3(0,0,b))-B(e-vec3(0,0,b))));}vec4 E(vec3 e,vec3 f,vec3 g,vec3 h,vec3 i,vec3 j,vec3 k,vec3 l){vec3 m,n,q,r,s,t;m=j;n=normalize(h-f);if(n.z>0.)return vec4(m,1);float o,p;o=length(h-f);p=1./(1.+.03*o+.003*o*o);q=normalize(f-e);r=k*max(dot(g,n),0.);s=reflect(n,g);t=l*pow(max(dot(s,q),0.),4.);m+=p*i*(r+t);return vec4(m,1);}void main(){vec3 e,f,h,j,l;e=vec3(0,0,-20.);f=vec3(-10.);vec2 g=gl_FragCoord.xy/vec2(s[1]/2.,s[2]/2.)-vec2(1);g.x*=s[1]/s[2];h=vec3(g.x,g.y,1);h=normalize(h)*.5;vec4 i=C(e,h);j=i.xyz;float k=i.w;l=D(j);if(k>0.){vec3 m,n,o,p;m=vec3(3);n=vec3(.3,0,0);o=vec3(1,0,0);p=vec3(1,1,0);vec4 q=E(e,j,l,f,m,n,o,p);float r=1.-max(distance(e,j)/120.,0.);gl_FragColor=q*r*k;}else if(s[0]>15.){vec3 m=v(vec3(g,0),.2*s[0]);gl_FragColor=vec4(sin(s[0])*cos(4.*m.x*m.y)+s[3]);}else gl_FragColor=vec4(0);gl_FragColor*=1.-smoothstep(56.,65.,s[0]);}";
#endif

#if 0

/* A crude minified version (glslunit.appspot.com/compiler.html)
 * Because of reasons.. :) .. I'll be using this tool for the Assembly
 * 13 entry. In the future, I need to install a shader minifier
 * locally!
 */
/* Due to reasons, for which I need to give some serious feedback to
 * Assembly organizers, unfortunately, I had to hack this manually in
 * the end, so it is functionally different from the unobfuscated one
 * (This remains here for historical reference; After the competition,
 * I made the equivalent non-minified shader below that works the same
 * way as the new minified version.):
 */
const GLchar *fs= "\
uniform float s[9];vec3 u(vec3 e,float f){return vec3(sin(f)*e.x-cos(f)*e.z,e.y,cos(f)*e.x+sin(f)*e.z);}vec3 v(vec3 e,float f){return vec3(sin(f)*e.x-cos(f)*e.y,cos(f)*e.x+sin(f)*e.y,e.z);}vec3 w(vec3 e,float f){return vec3(e.x,sin(f)*e.y-cos(f)*e.z,cos(f)*e.y+sin(f)*e.z);}float x(vec3 e,vec2 f){vec2 g=vec2(length(e.xy)-f.x,e.z);return length(g)-f.y;}float y(vec3 e,vec2 f){e.y-=abs(e.x);return x(e,vec2(f.x,f.y));}float z(vec3 e){e=v(e,s[0]*.1);e=w(e,s[0]*.2);e=u(e,s[0]);float f,g;f=y(e+vec3(7,0,0),vec2(5.+s[6],1));e=w(e,3.14+sin(s[0]));g=y(e,vec2(7.+s[6]*2.,1));return min(f,g);}float A(vec3 e){e=u(e,s[0]);float f=y(e,vec2(7.+s[6]*2.,1));return f;}float B(vec3 e){if(s[0]<29.6)return A(e);else return z(e);}const float a=.01;const float b=.1;const int c=180;const float d=80.;vec4 C(vec3 e,vec3 f){float g=0.;int h;vec3 i;for(h=0;h<c;h++){i=e+g*f;float j=B(i)*.9;g+=j;if(j<a)break;if(g>d)return vec4(i,0);}vec4 j;j.xyz=i;j.w=1.-float(h)/float(c);return j;}vec3 D(vec3 e){return normalize(vec3(B(e+vec3(b,0,0))-B(e-vec3(b,0,0)),B(e+vec3(0,b,0))-B(e-vec3(0,b,0)),B(e+vec3(0,0,b))-B(e-vec3(0,0,b))));}vec4 E(vec3 e,vec3 f,vec3 g,vec3 h,vec3 i,vec3 j,vec3 k,vec3 l){vec3 m,n,q,r,s,t;m=j;n=normalize(h-f);if(n.z>0.)return vec4(m,1);float o,p;o=length(h-f);p=1./(1.+.03*o+.003*o*o);q=normalize(f-e);r=k*max(dot(g,n),0.);s=reflect(n,g);t=l*pow(max(dot(s,q),0.),4.);m+=p*i*(r+t);return vec4(m,1);}void main(){vec3 e,f,h,j,l;e=vec3(0,0,-20.);f=vec3(-10.);vec2 g=gl_FragCoord.xy/vec2(s[1]/2.,s[2]/2.)-vec2(1);g.x*=s[1]/s[2];h=vec3(g.x,g.y,1);h=normalize(h)*.5;vec4 i=C(e,h);j=i.xyz;float k=i.w;l=D(j);if(k>0.){vec3 m,n,o,p;m=vec3(3);n=vec3(.3,0,0);o=vec3(1,0,0);p=vec3(1,1,0);vec4 q=E(e,j,l,f,m,n,o,p);gl_FragColor=q*k;}else if(s[0]>15.){vec3 m=v(vec3(g,0),.2*s[0]);gl_FragColor=vec4(sin(s[0])*cos(4.*m.x*m.y)+s[3]);}else gl_FragColor=vec4(0);gl_FragColor*=1.-smoothstep(56.,65.,s[0]);}";
#endif

#if 0
const GLchar *fs= "\
  uniform float s[9]; // State parameters from app.                     \
                                                                        \
// FIXME: Should do proper rotation matrices?                           \
// (my axes can be the wrong way around..                               \
// I made this 'out from my head')                                      \
vec3 rotY(vec3 p, float th){                                            \
  float si = sin(th);                                                   \
  float co = cos(th);                                                   \
  return vec3(si*p.x - co*p.z, p.y, co*p.x + si * p.z);                 \
}                                                                       \
                                                                        \
vec3 rotZ(vec3 p, float th){                                            \
  float si = sin(th);                                                   \
  float co = cos(th);                                                   \
  return vec3(si*p.x - co*p.y, co*p.x + si * p.y, p.z);                 \
}                                                                       \
                                                                        \
vec3 rotX(vec3 p, float th){                                            \
  float si = sin(th);                                                   \
  float co = cos(th);                                                   \
  return vec3(p.x, si*p.y - co*p.z, co*p.y + si * p.z);                 \
}                                                                       \
                                                                        \
float sdTorus( vec3 p, vec2 t )                                         \
{                                                                       \
  vec2 q = vec2(length(p.xy)-t.x,p.z);                                  \
  return length(q)-t.y;                                                 \
}                                                                       \
                                                                        \
// A heart-shape - just a bent torus                                    \
float heart(vec3 p, vec2 wt)                                            \
{                                                                       \
  p.y -= abs(p.x);                                                      \
  return sdTorus(p, vec2(wt.x,wt.y));                                   \
}                                                                       \
                                                                        \
float h(vec3 p){                                                        \
  p = rotZ(p,s[0]*.1);                                                  \
  p = rotX(p,s[0]*.2);                                                  \
  p = rotY(p,s[0]);                                                     \
  float f1 = heart(p+vec3(4.,0.,0.), vec2(5.,1.));                      \
  p = rotX(p,3.14+sin(s[0]*.2));                                        \
  float f2 = heart(p, vec2(5.+s[6]*2.,1.));                             \
  return min(f1,f2);                                                    \
}                                                                       \
                                                                        \
float f_intro(vec3 p){                                                  \
  p = rotY(p,s[0]);                                                     \
  float f1 = heart(p, vec2(7.+s[6]*2.,1.));                             \
  return f1;                                                            \
}                                                                       \
	                                                                      \
float f(vec3 p){                                                        \
  // My synth almost supports full synchronization of everything in the \
  // animation, but this production still needs hard-coded time sync.   \
  // This is very unfortunate, but I'll make it better before my next   \
  // demoscene entry...                                                 \
  if (s[0]<29.6){                                                       \
    return f_intro(p);                                                  \
  }else{                                                                \
    return h(p);                                                        \
  }                                                                     \
}                                                                       \
                                                                        \
                                                                        \
const float MinimumDistance = .01; // FIXME: accuracy vs. frame rate?   \
const float epsilon = 0.1;                                              \
const int MaxRaySteps = 180;                                            \
const float TooFar = 80.0;                                              \
                                                                        \
vec4 march(vec3 from, vec3 direction) {                                 \
  float totalDistance = 0.0;                                            \
  int steps;                                                            \
  vec3 p;                                                               \
  for (steps=0; steps < MaxRaySteps; steps++) {                         \
    p = from + totalDistance * direction;                               \
    float distance = f(p)*.9;                                           \
    totalDistance += distance;                                          \
    if (distance < MinimumDistance){                                    \
      break;                                                            \
    }                                                                   \
    if (totalDistance > TooFar) return vec4(p,0.0);                     \
  }                                                                     \
  vec4 res; res.xyz = p;                                                \
  res.w = 1.-float(steps)/float(MaxRaySteps);                           \
  return res;                                                           \
}                                                                       \
                                                                        \
// Distance estimation describes a hyperplane (local tangent plane of   \
// a geometry) in an implicit form (f(p)==0 exactly on the plane,       \
// f(p)>0 on the front side). Gradient of the function gives a vector   \
// orthogonal to the plane. Here we approximate the gradient with finite \
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
    vec3 lightPosition = vec3(-10.);                                    \
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
    //    if (length(pix) < 0.01){                                      \
    //      gl_FragColor = vec4(1.0,1.0,1.0,0.5);                       \
    //      return;                                                     \
    //    }                                                             \
    // Lighting computation.                                            \
    if (r>0.0){                                                         \
      vec3 lightC = vec3(3.);                                           \
      vec3 ambient = vec3(0.3,0.0,0.0);                                 \
      vec3 diffuse = vec3(1.0,0.0,0.0);                                 \
      vec3 specular = vec3(1.0,1.0,0.0);                                \
                                                                        \
      vec4 color = doLightPhong(cameraPosition, p, n,                   \
                             lightPosition,                             \
                             lightC,ambient,diffuse,specular);          \
                                                                        \
      float darken = 1.0-max((distance(cameraPosition,p)/120.0),0.0);   \
      gl_FragColor = color * darken * r;                                \
    } else {                                                            \
      // Could have some action in the background:                      \
      if (s[0]>15.){                                                    \
        vec3 bgp = rotZ(vec3(pix,0.),.2*s[0]);                          \
        gl_FragColor = vec4(sin(s[0])*cos(4.*bgp.x*bgp.y)+s[3]);        \
      } else {                                                          \
        gl_FragColor = vec4(0.);                                        \
      }                                                                 \
    }                                                                   \
    // Fade out at the end:                                             \
    gl_FragColor*=1.-smoothstep(56.,65.,s[0]);                          \
  }";

#endif
