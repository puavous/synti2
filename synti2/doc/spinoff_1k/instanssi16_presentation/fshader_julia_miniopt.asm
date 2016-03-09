minimized_fragment_shader_str:
 	db '#version 120',10
	db 'uniform vec4 u;'
	db 'float j(vec2 c,vec2 z,int i){'
	db    'for(i=i;i>0 && dot(z,z)<120;i--){'
	db	'z=vec2((z.x*z.x-z.y*z.y)+c.x,'
	db             '(z.y*z.x+z.x*z.y)+c.y);'
	db     '}'
	db   'return i/120.;'
	db '}'
	db 'void main(){'
	db    'vec2 v=1-gl_FragCoord.xy/(u.yz/2);'
	db    'v.x*=u.y/u.z;'
	db    'gl_FragColor=vec4(j(v,vec2(.1),120));'
	db '}'
	db 10,0
	
