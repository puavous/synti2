/* Some simple ray marching experiments...
 * 
 * Yep. This is mostly copy-paste from Iñigo Quilez's work. Thanx,
 * dude, even if I don't know you (yet) ;).
 *
 * But mind our students: I couldn't have done this without the basic
 * math and methodology which was taught on our course this spring!
 */
#define NEED_DEBUG
const GLchar *vs="\
  uniform float s[9]; // State parameters from app.                     \
  varying vec4 v; // Vertex coordinates                                 \
  void main(){                                                          \
    v = gl_Vertex;                                                      \
    gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * v;         \
  }";


const GLchar *fs= "\
  uniform float s[9]; // State parameters from app.                     \
  varying vec4 v; // Vertex coordinates                                 \
                                                                        \
float distanceEstimator(vec3 p){                                        \
    //vec3 c = vec3(sin(s[0])*3.0,0.0,0.0);                             \
    vec3 c = vec3(0.0,2.0,0.0);                                        \
    float r = 6.0+3.0*s[4];                                             \
    return length(p-c) -r;                                              \
}                                                                       \
                                                                        \
float sdHexPrism( vec3 p, vec2 h )                                      \
{                                                                       \
    vec3 q = abs(p);                                                    \
    return max(q.z-h.y,max(q.x+q.y*0.57735,q.y*1.1547)-h.x);            \
}                                                                       \
                                                                        \
float udRoundBox( vec3 p, vec3 b, float r )                             \
{                                                                       \
  return length(max(abs(p)-b,0.0))-r;                                   \
}                                                                       \
                                                                        \
float deRep( vec3 p, vec3 c )                                           \
{                                                                       \
    vec3 q = mod(p,c) - 0.5*c;                                          \
    //return distanceEstimator(q);                                        \
    return udRoundBox(q,vec3(3.0,3.0,0.2),0.5);                         \
}                                                                       \
                                                                        \
// FIXME: Rotation matrices...                                          \
vec3 rotY(vec3 p, float th){                                            \
  return vec3(sin(th)*p.x-cos(th)*p.z,                                  \
              p.y,                                                      \
              cos(th)*p.x+sin(th)*p.z);                                 \
}                                                                       \
                                                                        \
float g(vec3 p)                                                         \
{                                                                       \
    float distance;                                                     \
		//distance = distanceEstimator(p);                                  \
    distance = deRep(p,vec3(10.0,10.0,13.0));                        \
    // distance = sdHexPrism(p,vec2(4.0,7.0));                          \
    //distance = udRoundBox(p,vec3(3.0,3.0,0.2),0.5);                     \
    return distance;                                                    \
}                                                                       \
                                                                        \
float rotaTestY(vec3 p, float th){                                      \
  return g(rotY(p,th));                                                 \
}                                                                       \
                                                                        \
float f(vec3 p){                                                        \
  return rotaTestY(p,s[0]*0.2);                                         \
  //return g(p);                                                        \
}                                                                       \
                                                                        \
                                                                        \
vec4 march(vec3 from, vec3 direction) {                                 \
	float totalDistance = 0.0;                                            \
  float MinimumDistance = 0.001;                                         \
  int MaxRaySteps = 80;                                                  \
	int steps;                                                            \
  vec3 p;                                                               \
	for (steps=0; steps < MaxRaySteps; steps++) {                         \
		p = from + totalDistance * direction;                               \
    float distance = f(p);                                              \
		totalDistance += distance;                                          \
		if (distance < MinimumDistance) break;                              \
	}                                                                     \
  vec4 res; res.xyz = p;                                                \
  res.w = 1.0-float(steps)/float(MaxRaySteps);                          \
	return res;                                                           \
}                                                                       \
                                                                        \
vec3 normalEstimation(vec3 p){                                          \
  float epsilon = 0.01;                                                 \
  return normalize( vec3(                                               \
      f(p + vec3(epsilon,0,0) ) - f(p - vec3(epsilon,0,0))              \
    , f(p + vec3(0,epsilon,0) ) - f(p - vec3(0,epsilon,0))              \
    , f(p + vec3(0,0,epsilon) ) - f(p - vec3(0,0,epsilon))              \
) );}                                                                   \
                                                                        \
                                                                        \
// Phong model                                                          \
vec4 doLight(vec3 pcam, vec3 p, vec3 n, vec3 lpos,                      \
             vec3 lightC, vec3 amb, vec3 dfs, vec3 spec)                \
{                                                                       \
  vec3 ldir = normalize(lpos - p);                                      \
  // Ambient component:                                                 \
  vec3 c = amb;                                                         \
  // Diffuse and specular component:                                    \
  float ldist = length(lpos-p);                                         \
  float attn = 1.0 / (0.25*ldist + 0.06*ldist + 0.003*ldist*ldist);     \
  vec3 camdir = normalize(p-pcam);                                      \
  vec3 idfs = dfs * max(dot(n,ldir),0.0);                               \
  vec3 refldir = reflect(ldir, n);                                      \
  vec3 ispec = spec * max(dot(refldir,camdir),0.0);                      \
  c += attn * lightC * (idfs + ispec);                                  \
  return vec4 (c,1.0); // no alpha blending in use...                   \
}                                                                       \
                                                                        \
  void main(){                                                          \
    vec3 cameraPosition = vec3(0.0,0.0,-10.0);                          \
    vec3 lightPosition = vec3(10.0*sin(s[0]),20.0*sin(3.0*s[0]),10.0*cos(s[0])); \
    vec3 vto = vec3(v.x,v.y,2.0);                                       \
    vec3 vdir = normalize(vto);                                         \
    vec4 pr = march(cameraPosition,vdir);                               \
    vec3 p = pr.xyz;                                                    \
    float r = pr.w;                                                     \
    vec3 n = normalEstimation(p);                                       \
    // Lighting computation.                                            \
    if (r>0.0){                                                         \
      vec3 lightC = vec3(1.0,1.0,1.0);                                  \
      vec3 ambient = vec3(0.2,0.0,0.7);                                 \
      vec3 diffuse = vec3(0.7,0.2,0.1);                                 \
      vec3 specular = vec3(1.0,1.0,0.0);                                \
                                                                        \
      vec4 color = doLight(cameraPosition, p, n,                        \
                             lightPosition,                             \
                             lightC,ambient,diffuse,specular);          \
                                                                        \
      //color *= 20.0 / min(20.0 - length(cameraPosition-p),0.0);       \
      gl_FragColor = color;                                             \
    } else {                                                            \
      gl_FragColor = vec4(0.0,0.0,0.0,0.0);                             \
    }                                                                   \
  }";
