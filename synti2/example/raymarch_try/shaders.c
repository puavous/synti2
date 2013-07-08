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
#define NEED_DEBUG
const GLchar *vs="\
  void main(){                                                          \
    gl_Position = ftransform();         \
    //gl_Position = gl_Vertex;            \
  }";

#if 0

const GLchar *vs="\
  uniform float s[9]; // State parameters from app.                     \
  varying vec4 a; // Vertex coordinates                                 \
  void main(){                                                          \
    a = gl_Vertex;                                                      \
    gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * a;         \
  }";

const GLchar *fs= "\
uniform float s[9];varying vec4 a;vec3 u(vec3 e,float f){f+=1.+s[5];return vec3(sin(f)*e.x-cos(f)*e.z,e.y,cos(f)*e.x+sin(f)*e.z);}float v(vec3 e,vec2 f){vec3 g=abs(e);return max(g.z-f.y,max(g.x+g.y*.57735,g.y*1.1547)-f.x);}float w(vec3 e,vec3 f,float g){vec3 h=abs(e)-f;return min(max(h.x,max(h.y,h.z)),0.)+length(max(h,0.))-g;}float x(vec3 e,vec3 f,float g){vec3 h=u(e,e.y*sin(s[0])*.2);return w(h,f,g);}float y(vec3 e,vec3 f){vec3 g=mod(e,f)-.5*f;return x(g,vec3(3,3,2.2),.5);}float z(vec3 e){float f;f=y(e,vec3(12,12,15));f=min(f,v(e,vec2(4,7)));return f;}float A(vec3 e,float f){return z(u(e,f));}float B(vec3 e){return A(e,s[0]*.2);}const float b=.001;const int c=140;const float d=120.;vec4 C(vec3 e,vec3 f){float g=0.;int h;vec3 i;for(h=0;h<c;h++){i=e+g*f;float j=B(i);g+=j;if(j<b)break;if(g>d)return vec4(i,0);}vec4 j;j.xyz=i;j.w=1.-float(h)/float(c);return j;}vec3 D(vec3 e){float f=.001;return normalize(vec3(B(e+vec3(f,0,0))-B(e-vec3(f,0,0)),B(e+vec3(0,f,0))-B(e-vec3(0,f,0)),B(e+vec3(0,0,f))-B(e-vec3(0,0,f))));}vec4 E(vec3 e,vec3 f,vec3 g,vec3 h,vec3 i,vec3 j,vec3 k,vec3 l){vec3 m,n,q,r,s,t;m=normalize(h-f);n=j;float o,p;o=length(h-f);p=1./(1.+.06*o+.003*o*o);q=normalize(f-e);r=k*max(dot(g,m),0.);s=reflect(m,g);t=l*pow(max(dot(s,q),0.),4.);n+=p*i*(r+t);return vec4(n,1);}void main(){vec3 e,f,g,h,i,k,m;e=vec3(0,0,-10.);f=vec3(20.*sin(s[0]),20.*cos(s[0]),10.*cos(s[0]*.1));g=vec3(2.*sin(s[0]),2.*cos(s[0]),20.*cos(s[0]*.4));h=vec3(a.x,a.y,2);i=normalize(h);vec4 j=C(e,i);k=j.xyz;float l=j.w;m=D(k);if(l>0.){vec3 n,o,p,q;n=vec3(1);o=vec3(.1,0,.2);p=vec3(.2,.1,.05);q=vec3(1,1,0);vec4 r=E(e,k,m,f,n,o,p,q);r+=E(e,k,m,g,n,o,p,q);float s=1.-max(distance(e,k)/120.,0.);gl_FragColor=3.*r*s;}else gl_FragColor=vec4(0);}";
#endif

#if 1
const GLchar *fs= "\
  uniform float s[9]; // State parameters from app.                     \
                                                                        \
float distanceEstimator(vec3 p){                                        \
    vec3 c = vec3(0.0,0.0,0.0);                                         \
    float r = 6.0+3.0*s[4];                                             \
    return length(p-c) -r;                                              \
}                                                                       \
                                                                        \
// FIXME: Should do proper rotation matrices...                         \
vec3 rotY(vec3 p, float th){                                            \
  th += (1.0+s[5]);                                                     \
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
float sdHexPrism( vec3 p, vec2 h )                                      \
{                                                                       \
    vec3 q = abs(p);                                                    \
    return max(q.z-h.y,max(q.x+q.y*0.57735,q.y*1.1547)-h.x);            \
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
  vec3 rp = rotY(p,p.y*sin(s[0])*0.2);                                  \
  return sdRoundBox(rp,b,r);                                            \
}                                                                       \
                                                                        \
float deRep( vec3 p, vec3 c )                                           \
{                                                                       \
    vec3 q = mod(p,c) - 0.5*c;                                          \
    //return distanceEstimator(q);                                      \
    return warpedRoundBox(q,vec3(3.0,3.0,2.2),0.5);                     \
    //return sdHexPrism(q,vec2(3.0,0.2));                               \
}                                                                       \
                                                                        \
                                                                        \
                                                                        \
float g(vec3 p)                                                         \
{                                                                       \
    //p = rotY(rotZ(p,s[0]),s[0]/2.);                                     \
    float distance;                                                     \
		//distance = distanceEstimator(p);                                  \
    distance = deRep(p,vec3(12.0,12.0,15.0));                           \
    distance = min(distance,sdHexPrism(rotY(rotZ(p,s[0]),s[0]/2.),vec2(4.0,7.0)));               \
    //distance = sdRoundBox(p,vec3(3.0,3.0,0.2),0.5);                   \
    return distance;                                                    \
}                                                                       \
                                                                        \
float rotaTestY(vec3 p, float th){                                      \
  return g(rotY(p,th));                                                 \
  //return g(rotY(rotZ(p,s[0]),th)+vec3(0.0,0.0,s[0]*4.0));             \
}                                                                       \
                                                                        \
float f(vec3 p){                                                        \
  // Muutama erilainen pikkukokeilu..                                   \
  //return rotaTestY(p,s[0]*0.2);                                         \
  //return distanceEstimator(p);                                        \
  return g(p);                                                        \
}                                                                       \
                                                                        \
                                                                        \
                                                                        \
const float MinimumDistance = 0.01;                                     \
const int MaxRaySteps = 100;                                            \
const float TooFar = 80.0;                                              \
                                                                        \
vec4 march(vec3 from, vec3 direction) {                                 \
	float totalDistance = 0.0;                                            \
	int steps;                                                            \
  vec3 p;                                                               \
	for (steps=0; steps < MaxRaySteps; steps++) {                         \
		p = from + totalDistance * direction;                               \
    float distance = f(p) *0.9;                                         \
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
  float epsilon = 0.1;                                                 \
  return normalize( vec3(                                               \
      f(p + vec3(epsilon,0.,0.) ) - f(p - vec3(epsilon,0.,0.))              \
    , f(p + vec3(0.,epsilon,0.) ) - f(p - vec3(0.,epsilon,0.))              \
    , f(p + vec3(0.,0.,epsilon) ) - f(p - vec3(0.,0.,epsilon))              \
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
  float attn = 1.0 / (1.0 + 0.06*ldist + 0.003*ldist*ldist);            \
  vec3 camdir = normalize(p-pcam);                                      \
  vec3 idfs = dfs * max(dot(n,ldir),0.0);                               \
  vec3 refldir = reflect(ldir, n);                                      \
  vec3 ispec = spec * pow(max(dot(refldir,camdir),0.0),4.0);            \
  c += attn * lightC * (idfs + ispec);                                  \
  return vec4 (c,1.0); // no alpha blending in use...                   \
}                                                                       \
                                                                        \
  void main(){                                                          \
    vec3 cameraPosition = vec3(0.0,0.0,-20.0);                          \
    vec3 lightPosition = vec3(20.0*sin(s[0]),20.0*cos(s[0]),10.0*cos(s[0]*0.1)); \
    vec3 lightPosition2 = vec3(2.0*sin(s[0]),2.0*cos(s[0]),20.0*cos(s[0]*0.4)); \
                                                                        \
    // I just shoot 'over there'. TODO: Use screen pixel coordinates.   \
    // TODO: Proper vector length and direction.                        \
    vec2 pix = gl_FragCoord.xy / vec2(400.0,300.0) - vec2(1.,1.);       \
    vec3 vto = vec3(pix.x,pix.y,1.);                                   \
    vec3 vdir = normalize(vto);                                         \
                                                                        \
    vec4 pr = march(cameraPosition,vdir);                               \
    vec3 p = pr.xyz;                                                    \
    float r = pr.w;                                                     \
    vec3 n = normalEstimation(p);                                       \
    // Lighting computation.                                            \
    if (r>0.0){                                                         \
      vec3 lightC = vec3(1.);                                  \
      vec3 ambient = vec3(0.1,0.0,1.0);                                 \
      vec3 diffuse = vec3(1.0,0.1,0.05);                                \
      vec3 specular = vec3(1.0,1.0,0.0);                                \
                                                                        \
      vec4 color = doLightPhong(cameraPosition, p, n,                   \
                             lightPosition,                             \
                             lightC,ambient,diffuse,specular);          \
                                                                        \
      color += doLightPhong(cameraPosition, p, n,                       \
                             lightPosition2,                            \
                             lightC,ambient,diffuse,specular);          \
                                                                        \
      float darken = 1.0-max((distance(cameraPosition,p)/120.0),0.0);   \
      gl_FragColor = 3.0*color * darken;                                    \
    } else {                                                            \
      gl_FragColor = vec4(0.);                             \
    }                                                                   \
  }";

#endif
