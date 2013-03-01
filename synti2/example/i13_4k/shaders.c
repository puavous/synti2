/* So far just a placeholder. I admit, this is the first example from
 * http://www.lighthouse3d.com/tutorials/glsl-tutorial/hello-world-in-glsl/
 */
//#define NEED_DEBUG
const GLchar *vs="\
//#version 120\n\
  uniform float s[16]; // State parameters from app.\
  varying vec3 n;  // Normal.\
  varying vec4 po; // A test of my own.. for education \
  void main(){\
    vec4 t=vec4(sin(s[0]),cos(s[0]), 30.0, 0.0);\
    po = gl_Vertex;\
    gl_Position = gl_ProjectionMatrix * ((gl_ModelViewMatrix * ((5.0+s[9])*po)-t));\
    gl_FrontColor = vec4(1.0,po.x,po.y,1.0);\
    // Here's my normal, if I need it after all:\
    n = gl_NormalMatrix * vec3(0.0,0.0,1.0);        \
  }";


const GLchar *fs= "\
  varying vec3 n; // normal for lights\
  varying vec4 po; // Vertex coordinates\
  uniform float s[16]; // State parameters from app.\
  void main(){\
    float d=sqrt((po.x*po.x+po.y*po.y));\
    float h=1.4+sin(30.0+d*sin(s[0])*sin(s[0])*20.0);\
    if (h<0.5) discard;\
    vec4 c = gl_Color;\
    c.a = h;\
    vec3 ld = normalize(vec3(0.0,0.0,0.5));\
    float li = dot(ld, normalize(n));\
    c.g = li;\
    gl_FragColor = c;\
  }";
