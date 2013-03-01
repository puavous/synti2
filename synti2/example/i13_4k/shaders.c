/* So far just a placeholder. I admit, this is the first example from
 * http://www.lighthouse3d.com/tutorials/glsl-tutorial/hello-world-in-glsl/
 */
//#define NEED_DEBUG
const GLchar *vs="\
  varying vec3 n;  // Normal.\
  varying vec4 po; // A test of my own.. for education \
  void main(){\
    gl_Position = ftransform();\
    // That is essentially the same as \
    //gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex; \
\
    // .. And we can do translations and stuff in here:\
    gl_Position = gl_ProjectionMatrix * (gl_ModelViewMatrix * 10.0*gl_Vertex-vec4(0.0,0.0,40.0,0.0)); \
    // Mmm. We could set colors here, if we wanted:\
    gl_FrontColor = vec4(1.0,0.0,0.0,1.0);\
    //gl_FrontColor = vec4(0.5+0.5*sin(gl_Vertex.x+2.0),0.0,0.5+0.5*sin(gl_Vertex.y+2.0),1.0);\
\
    // We could compute per-vertex lights:  \
    //vec3 ld = normalize(vec3(-1.0,1.0,0.5));\
    //vec4 n = vec4(gl_Normal,0.0);\
    //n = normalize(gl_ModelViewMatrix * n);\
    //li = dot(ld,vec3(n.x,n.y,n.z));\
    //li = dot(ld, b);\
\
    // Or just transport the normal to vertex for light comput.:\
    //n = gl_NormalMatrix * gl_Normal;\
    // Which in our case is always this:
    n = gl_NormalMatrix * vec3(0.0,0.0,1.0);        \
    po = gl_Vertex;\
  }";

const GLchar *fs= "\
  //varying float li; // Light intensity for frags\
  varying vec3 n; // normal for lights\
  varying vec4 po; // A test of my own.. for education\
  void main(){\
    // Mmm.. We could use the color transmitted from the vertex shader:\
    float hm=(po.x*po.x+po.y*po.y);\
    //if (hm<0.001) discard;\
    vec4 c = gl_Color;\
    c.a = sin(hm*7.0); //0.9; //max(hm,1.0);\
    vec3 ld = normalize(vec3(-1.0,1.0,0.5));\
    float li = dot(ld, normalize(n));\
    c.g = li;\
    gl_FragColor = c;\
  }";
