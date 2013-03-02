/* So far just a placeholder. I admit, this is the first example from
 * http://www.lighthouse3d.com/tutorials/glsl-tutorial/hello-world-in-glsl/
 */
//#define NEED_DEBUG
const GLchar *vs="\
  uniform float s[9]; // State parameters from app.                     \
  //varying vec3 n;  // Normal.                                         \
  varying vec4 v; // Vertex coordinates                                 \
  void main(){                                                          \
    v = gl_Vertex;                                                      \
    vec4 t=vec4(0,0,15,0);                                              \
    float k=1.0; // 'Kick'                                              \
    if (v.z >= 0.0){                                                     \
      k+=s[1];                                                          \
      v.z=1.0+(v.z*(1.0-cos(s[0])));                                    \
    }                                                                   \
    //t.z=15.0;                                                         \
    gl_Position = gl_ProjectionMatrix * (gl_ModelViewMatrix*k*v - t);   \
    //gl_FrontColor = vec4(1.0,v.x,v.y,1.0);                            \
    // Here's my normal, if I need it after all:                        \
    //n = gl_NormalMatrix * vec3(0.0,0.0,1.0);                          \
  }";

#if 0

    if (v.z < -9.0){                                                   \
      t.x=sin(s[0]);t.y=cos(s[0]);\                                     \
      //t=vec4(sin(s[0]),cos(s[0]), 0, 0);                              \
    } else if (v.z < -8.0){                                            \
      t.x=sin(3.0*s[0]);t.y=cos(5.0*s[0]);\                             \
      //t=vec4(sin(3.0*s[0]),cos(5.0*s[0]), 0, 0);                      \
    } else if (v.z < -7.0) {                                           \
      t.x=sin(5.0*s[0]);t.y=cos(3.0*s[0]);\                             \
      //t=vec4(sin(5.0*s[0]),cos(s[0]+3.0), 0, 0);                      \
    } else {                                                            \
      k+=s[1];                                                          \
      v.z=1.0+(v.z*(1.0-cos(s[0])));                                  \
    }                                                                   \

#endif

const GLchar *fs= "\
  uniform float s[9]; // State parameters from app. \
  //varying vec3 n; // normal for lights            \
  varying vec4 v; // Vertex coordinates             \
  void main(){                                      \
    float d = length(vec2(v));                      \
    if (d>1.0) if (v.z>0.0) discard;                \
\
    //float h = sin(10.0*d-2.0*s[0]);                      \
\
    float h = sin(10.0*(d-s[0]));                 \
    //float h = sin(2.0*s[0]);                       \
                                                    \
    vec4 c = vec4(1.0,v.x,v.y,1.0-h);                 \
    c.r = sin(s[0]+v.z);                            \
    c.g = v.x-sin(v.y+s[0]);                        \
    //c.a = 1.0-h; //sqrt(h);                           \
\
    if (v.z >= 0.0) c.a /= 2.0; else c /=4.0;                       \
//    if (v.z < 0.0) c *= .25;                        \
    //vec3 ld = normalize(vec3(0.0,0.0,0.5));       \
    //float li = dot(ld, normalize(n));             \
    //c.g = li;                                     \
    gl_FragColor = c;                               \
  }";
