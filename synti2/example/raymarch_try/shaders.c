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
    //vec3 c = vec3(sin(s[0]*0.1)*12.0,0.0,0.0);                        \
    vec3 c = vec3(sin(s[0]*0.1)*3.0,0.0,0.0);                          \
    float r = 6.0+sin(s[0])+s[1];                                       \
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
    return distanceEstimator(q);                                        \
}                                                                       \
                                                                        \
float f(vec3 p)                                                         \
{                                                                       \
		float distance = distanceEstimator(p);                              \
    //float distance = deRep(p,vec3(10.0,10.0,10.0));                   \
    //float distance = sdHexPrism(p,vec2(3.0,4.0));                     \
    //float distance = udRoundBox(p,vec3(3.0,2.0,4.0),0.5);             \
    return distance;                                                    \
}                                                                       \
                                                                        \
vec4 march(vec3 from, vec3 direction) {                                 \
	float totalDistance = 0.0;                                            \
  float MinimumDistance = 0.01;                                         \
  int MaxRaySteps = 10;                                                 \
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
vec4 doLight(vec3 p, vec3 n, vec3 dirL)                                 \
{                                                                       \
  float r = dot(n,dirL);                                                \
  vec4 c = vec4(r, 0.0, 0.0, 1.0);                                      \
  return c;                                                             \
  //gl_FragColor = c;                                                   \
  //return vec4(1.0,0.0,0.0,1.0);                                       \
}                                                                       \
  void main(){                                                          \
    vec3 vto = vec3(v.x,v.y,1.0);                                       \
    vec3 vdir = normalize(vto);                                         \
    vec4 pr = march(vec3(0.0,0.0,-10.0),vdir);                          \
    vec3 p = pr.xyz;                                                    \
    float r = pr.w;                                                     \
    vec3 n = normalEstimation(p);                                       \
    // Lighting computation.                                            \
    if (r>0.0){                                                         \
      vec3 dirLight = normalize(vec3(1.0,1.0,-1.0));                    \
      gl_FragColor = doLight(p,n,dirLight);                             \
      } else {                                                            \
      gl_FragColor = vec4(0.0,0.0,0.0,0.0);                             \
    }                                                                   \
  }";
