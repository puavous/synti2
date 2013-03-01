/* So far just a placeholder. I admit, this is the first example from
 * http://www.lighthouse3d.com/tutorials/glsl-tutorial/hello-world-in-glsl/
 */
//#define NEED_DEBUG
const GLchar *vs="\
  uniform float s[16]; // State parameters from app.                    \
  varying vec3 n;  // Normal.                                           \
  varying vec4 po; // A test of my own.. for education                  \
  void main(){                                                          \
    po = gl_Vertex;                                                     \
    vec4 t;                                                             \
    float k=1.0; // 'Kick'                                              \
    if (po.z < -9.0){                                                   \
      t=vec4(sin(s[0]),cos(s[0]), 10.0, 0.0);                           \
    } else if (po.z < -8.0){                                            \
      t=vec4(sin(3.0*s[0]),cos(5.0*s[0]), 10.0, 0.0);                   \
    } else if (po.z < -7.0) {                                           \
      t=vec4(sin(5.0*s[0]),cos(s[0]+3.0), 10.0, 0.0);                   \
    } else {                                                            \
      t=vec4(0.0,0.0,10.0,0.0);                                         \
      k=k+s[9];                                                         \
    }                                                                   \
    gl_Position = gl_ProjectionMatrix * ((gl_ModelViewMatrix * k * po-t)); \
    gl_FrontColor = vec4(1.0,po.x,po.y,1.0);                            \
    // Here's my normal, if I need it after all:                        \
    //n = gl_NormalMatrix * vec3(0.0,0.0,1.0);                          \
  }";


const GLchar *fs= "\
  uniform float s[16]; // State parameters from app.\
  varying vec3 n; // normal for lights\
  varying vec4 po; // Vertex coordinates\
  void main(){\
    float d = sqrt((po.x*po.x+po.y*po.y));\
    float h = sin(40.0*d-2.0*s[0]);\
    //if (h<0.2) discard;\
    vec4 c = gl_Color;\
    c.r = sin(s[0]+po.z);\
    c.g = po.x-sin(po.y+s[0]);\
    c.a = sqrt(h);\
    if (po.z >= 0.0) c.a = c.a / 2.0;\
    //vec3 ld = normalize(vec3(0.0,0.0,0.5));\
    //float li = dot(ld, normalize(n));\
    //c.g = li;\
    gl_FragColor = c;\
  }";
