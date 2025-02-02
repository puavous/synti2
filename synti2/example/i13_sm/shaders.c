/* So far just a placeholder. I admit, this is the first example from
 * http://www.lighthouse3d.com/tutorials/glsl-tutorial/hello-world-in-glsl/
 */
//#define NEED_DEBUG
const GLchar *vs="\
  uniform float s[24]; // State parameters from app.                     \
  varying vec4 v; // Vertex coordinates                                 \
  void main(){                                                          \
    v = gl_Vertex;                                                      \
    vec4 t = vec4(0,0,15,0);                                              \
    float k = 1.0; // 'Kick'                                              \
    if (v.z >= 0.0){                                                     \
      //k+=s[1];                                                          \
      k+=s[9];                                                          \
      //v.z += 1.0 - v.z*cos(s[0]);                                      \
      v.z += 1.0 - v.z*cos(0.1+s[19]);                                      \
    }                                                                   \
    gl_Position = gl_ProjectionMatrix * (gl_ModelViewMatrix*k*v - t);   \
  }";


const GLchar *fs= "\
  uniform float s[24]; // State parameters from app.                    \
  //varying vec3 n; // normal for lights                               \
  varying vec4 v; // Vertex coordinates                                \
  void main(){                                                         \
    float d = length(vec2(v));                                         \
    if (d>1.0) if (v.z>0.0) discard;                                   \
    //float h = sin(10.0*d-2.0*s[0]);                                  \
    float h = sin(10.0*(d-s[0]));                                      \
    vec4 c = vec4(sin(v.z),v.x-sin(v.y+s[0]),v.y,1.0-h);               \
    if (v.z >= 0.0) c.a /= 2.0; else c.a /=4.0;                        \
    c += s[4] + (v.z>0.0?0.0:3.0)*s[5];\
    //c *= (smoothstep(0,4,s[0])-smoothstep(70,74,s[0]));\
    gl_FragColor = c;                                                  \
  }";

