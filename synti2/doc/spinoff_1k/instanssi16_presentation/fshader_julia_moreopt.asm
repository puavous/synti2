minimized_fragment_shader_str:
 	db '#version 120',10
	db 'uniform vec4 u;'
	db 'void main(){'
	db    'vec2 x=1-gl_FragCoord.xy/u.yz*2;'
 	db    'x.x*=u.y/u.z;'
	db    'int y;'
	db    'for(y=120;y>0&&dot(x,x)<120;y--)'
	db	'x=vec2((x.x*x.x-x.y*x.y)+.7+sin(u.x)/3,'
	db             '(2*x.x*x.y)+.37+sin(u.x/4)/4);'
	db    'gl_FragColor=vec4(y/120.);'
	db '}'

	
