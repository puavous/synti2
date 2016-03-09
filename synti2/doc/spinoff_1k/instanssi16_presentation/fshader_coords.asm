minimized_fragment_shader_str:
 	db '#version 120',10
	db 'uniform vec4 u;'
	db 'void main(){'
	db    'vec2 v=u.yz;'
	db    'v=1-gl_FragCoord.xy/(v/2);'
	db    'v.x*=u.y/u.z;'
	db    'gl_FragColor=vec4(length(v)<.1?1:0);'
	db '}'
	db 10,0
	
