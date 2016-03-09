minimized_vertex_shader_str:
	db 'void main(){gl_Position=ftransform();}',10,0
minimized_fragment_shader_str:
	db '#version 120',10
	db 'uniform float s[200];void main(){gl_FragColor=vec4(sin(s[0]),.2,.1,1.);}',10,0
