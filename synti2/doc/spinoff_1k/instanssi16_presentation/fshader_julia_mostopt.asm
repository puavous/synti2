minimized_fragment_shader_str:
 	db '#version 120',10
	db 'uniform vec4 u;'
	db 'void main(){'
	db    'vec2 r=1-gl_FragCoord.rg/u.tp*2;'
 	db    'r.r*=u.g/u.p;'
	db    'int a=120;'
 	db    'while(a-->0&&dot(r,r)<120)'
	db	'r=vec2((r.r*r.r-r.t*r.t)+.4+sin(u.r)/3,'
	db             '(2*r.r*r.t)+.34+sin(u.r/4)/4);'
	db    'gl_FragColor=vec4(a/120.);'
	db '}'

	
