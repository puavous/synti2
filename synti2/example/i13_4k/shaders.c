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
  }";

const GLchar *fs= "\
  void main(){\
    gl_FragColor = vec4(0.4,0.4,0.8,1.0);\
  }";

