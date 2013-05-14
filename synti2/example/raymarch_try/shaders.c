/* So far just a placeholder. I admit, this is the first example from
 * http://www.lighthouse3d.com/tutorials/glsl-tutorial/hello-world-in-glsl/
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
		float distance = distanceEstimator(p);                            \
    //float distance = deRep(p,vec3(10.0,10.0,10.0));                   \
    //float distance = sdHexPrism(p,vec2(3.0,4.0));                     \
    //float distance = udRoundBox(p,vec3(3.0,2.0,4.0),0.5);               \
    return distance;                                                    \
}                                                                       \
                                                                        \
                                                                        \
vec4 trace(vec3 from, vec3 direction) {                                 \
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
  vec4 res; res.xyz = p;                                                     \
  res.w = 1.0-float(steps)/float(MaxRaySteps);                          \
	return res;                                                           \
}                                                                       \
                                                                        \
vec3 normalEstimation(vec3 p){                                         \
  float epsilon = 0.01;                                                 \
  return normalize( vec3(                                               \
      f(p + vec3(epsilon,0,0) ) - f(p - vec3(epsilon,0,0))              \
    , f(p + vec3(0,epsilon,0) ) - f(p - vec3(0,epsilon,0))              \
    , f(p + vec3(0,0,epsilon) ) - f(p - vec3(0,0,epsilon))              \
) );}                                                                   \
                                                                        \
  void main(){                                                          \
    vec3 vto = vec3(v.x,v.y,1.0);                                       \
    vec3 vdir = normalize(vto);                                         \
    vec4 pr = trace(vec3(0.0,0.0,-10.0),vdir);                          \
    vec3 p = pr.xyz;                                                    \
    float r = pr.w;                                                     \
    vec3 n = normalEstimation(p);                                       \
    // Lighting computation.                                            \
    if (r>0.0){                                                         \
      vec3 lightDir = normalize(vec3(1.0,1.0,-1.0));                     \
      r = dot(n,lightDir);                                              \
      vec4 c = vec4(r, r, r, 1.0);                                      \
      gl_FragColor = c;                                                 \
    } else {                                                            \
      gl_FragColor = vec4(0.0,0.0,0.0,0.0);                             \
    }                                                                   \
  }";//    if (v.z >= 0.0) c.a /= 2.0; else c.a /=4.0;                       \
//    c += s[4] + (v.z>0.0?0.0:3.0)*s[5];                               \
//    c *= (smoothstep(0,4,s[0])-smoothstep(70,74,s[0]));               \


