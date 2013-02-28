/* So far just a placeholder. I admit, this is the first example from
 * http://www.lighthouse3d.com/tutorials/glsl-tutorial/hello-world-in-glsl/
 */

const GLchar *vs="\
  varying float li; // Light intensity for frags\
  void main(){\
    //gl_Position = ftransform();\
    // That is essentially the same as \
    // gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex; \
    // .. And we can do translations and stuff in here:\
    gl_Position = gl_ProjectionMatrix * (gl_ModelViewMatrix * gl_Vertex - vec4(0.0,0.0,20.0,0.0)); \
    // Mmm. We could set colors here, if we wanted:\
    //gl_FrontColor = vec4(1.0,0.0,0.0,1.0);\
    gl_FrontColor = vec4(0.5+0.5*sin(gl_Vertex.x+2.0),0.0,0.5+0.5*sin(gl_Vertex.y+2.0),1.0);\
\
    // We could compute per-vertex lights:  \
    vec3 ld = normalize(vec3(-1.0,1.0,0.5));\
    vec4 n = vec4(gl_Normal,0.0);\
    n = normalize(gl_ModelViewMatrix * n);\
    li = dot(ld,vec3(n.x,n.y,n.z));\
    //li = dot(ld, b);\
  }";

const GLchar *fs= "\
  varying float li; // Light intensity for frags\
  void main(){\
    // Mmm.. We could use the color transmitted from the vertex shader:\
    vec4 c = gl_Color;\
    c.g = li;\
    gl_FragColor = c;\
  }";

