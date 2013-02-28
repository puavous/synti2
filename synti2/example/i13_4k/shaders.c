/* So far just a placeholder. I admit, this is the first example from
 * http://www.lighthouse3d.com/tutorials/glsl-tutorial/hello-world-in-glsl/
 */

const GLchar *vs="\
  void main(){\
    //gl_Position = ftransform();\
    // That is essentially the same as \
    // gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex; \
    // .. And we can do translations and stuff in here:\
    gl_Position = gl_ProjectionMatrix * (gl_ModelViewMatrix * gl_Vertex - vec4(0.0,0.0,20.0,0.0)); \
    // Mmm. We could set colors here, if we wanted:\
    //gl_FrontColor = vec4(1.0,0.0,0.0,1.0);\
    gl_FrontColor = vec4(0.5+0.5*sin(gl_Vertex.x+2.0),0.0,0.5+0.5*sin(gl_Vertex.y+2.0),1.0);\
  }";

const GLchar *fs= "\
  void main(){\
    // Mmm.. We could use the color transmitted from the vertex shader:\
    gl_FragColor = gl_Color;\
  }";

