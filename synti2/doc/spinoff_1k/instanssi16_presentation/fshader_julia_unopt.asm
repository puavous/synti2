minimized_fragment_shader_str:
 	db '#version 120',10
	db 'uniform vec4 u;'
	db 'float julia(vec2 c, vec2 z, int iter){'
	db    'vec2 h;'
	db    'int i;'
	db    'for(i=0; i<iter; i++) {'
	db	'h = vec2((z.x * z.x - z.y * z.y) + c.x,'
	db           '(z.y * z.x + z.x * z.y) + c.y);'
	db      'if(dot(h,h) > 76.0) break;'
	db      'z = h;'
	db     '}'
	db   'return ((i == iter) ? 0.0 : float(i)) / 100.0;'
	db '}'
	db 'void main(){'
	db    'vec2 v=u.yz;'
	db    'v=1-gl_FragCoord.xy/(v/2);'
	db    'v.x*=u.y/u.z;'
	db    'gl_FragColor=vec4(julia(v.xy,vec2(.1),60));'
	db '}'
	db 10,0
	
