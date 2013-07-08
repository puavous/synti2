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
#if 1

const GLchar *vs="\
  void main(){                                  \
    gl_Position = ftransform();                 \
    gl_Position.xy *= .5;                       \
  }";


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
float sdRoundBox( vec3 p, vec3 b, float r)                              \
{                                                                       \
  vec3 d = abs(p) - b;                                                  \
  return min(max(d.x,max(d.y,d.z)),0.0) +                               \
         length(max(d,0.0)) - r;                                        \
}                                                                       \
                                                                        \
float warpedRoundBox( vec3 p, vec3 b, float r )                         \
{                                                                       \
  vec3 rp = rotY(p,p.y*sin(s[0])*0.4+1.0+s[5]);                         \
  rp = rotX(rp,rp.z*sin(s[0]+1.0)*.1);                                  \
  return sdRoundBox(rp,b,r);                                            \
}                                                                       \
                                                                        \
vec3 deRep( vec3 p, vec3 c )                                            \
{                                                                       \
    return mod(p,c);                                                    \
}                                                                       \
                                                                        \
float g(vec3 p)                                                         \
{                                                                       \
    vec3 c = vec3(12.,12.,25.);                                         \
    p.z += 10.*s[0];                                                    \
    vec3 q = deRep(rotX(p,.1*s[0]),c)- 0.5*c;                           \
    return warpedRoundBox(q,vec3(3.0,3.0,2.2),0.5);                     \
}                                                                       \
                                                                        \
float f(vec3 p){                                                        \
  return g(p);                                                          \
}                                                                       \
                                                                        \
                                                                        \
const float MinimumDistance = .8; // FIXME: accuracy vs. frame rate    \
const float epsilon = 0.1;                                              \
const int MaxRaySteps = 140;                                            \
const float TooFar = 80.0;                                              \
                                                                        \
vec4 march(vec3 from, vec3 direction) {                                 \
	float totalDistance = 0.0;                                            \
	int steps;                                                            \
  vec3 p;                                                               \
	for (steps=0; steps < MaxRaySteps; steps++) {                         \
		p = from + totalDistance * direction;                               \
    float distance = f(p)*0.8;                                          \
		totalDistance += distance;                                          \
		if (distance < MinimumDistance){                                    \
      break;                                                            \
    }                                                                   \
    if (totalDistance > TooFar) return vec4(p,0.0);                     \
	}                                                                     \
  vec4 res; res.xyz = p;                                                \
  res.w = 1.0-float(steps)/float(MaxRaySteps);                          \
	return res;                                                           \
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
  vec3 ldir = normalize(lpos - p);                                      \
                                                                        \
  // Ambient component:                                                 \
  vec3 c = amb;                                                         \
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
    vec3 lightPosition = vec3(sin(s[0]),cos(s[0]*.4),cos(s[0]*.1));     \
    lightPosition *= 20.;                                               \
                                                                        \
    // I just shoot 'over there'.                                       \
    // TODO: Proper vector length and direction; from resol.            \
    vec2 pix = gl_FragCoord.xy / vec2(512.0,384.0) - vec2(1.,1.);       \
    vec3 vdir = vec3(pix.x,pix.y,1.);                                   \
    //vec3 vdir = normalize(vdir);                                      \
                                                                        \
    vec4 pr = march(cameraPosition,vdir);                               \
    vec3 p = pr.xyz;                                                    \
    float r = pr.w;                                                     \
    vec3 n = normalEstimation(p);                                       \
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
      gl_FragColor = 3.0*color * darken;                                \
    } else {                                                            \
      gl_FragColor = vec4(0.);                                          \
    }                                                                   \
  }";

#endif
