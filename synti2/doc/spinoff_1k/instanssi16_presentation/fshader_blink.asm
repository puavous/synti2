minimized_fragment_shader_str:
 	db '#version 120',10
	db 'uniform vec4 u;'
	db 'void main(){'
	db    'gl_FragColor=vec4(sin(u.x*3));'
	db '}'
	db 10,0
	
