minimized_fragment_shader_str:
 	db '#version 120',10
	db 'uniform vec4 u;'
	;; Use z.zw for storage, no need for local.
	db 'float j(vec2 c,vec4 z,int i){'
	db    'for(i=i;i>0 && dot(z.zw,z.zw)<76;i--) {'
	db	'z=vec4(0,0,(z.x*z.x-z.y*z.y)+c.x,'
	db                 '(z.y*z.x+z.x*z.y)+c.y);'
	db      'z.xy=z.zw;'
	db     '}'
	db   'return (i/100.);'
	db '}'
	db 'void main(){'
	db    'vec2 v=u.yz;'
	db    'v=1-gl_FragCoord.xy/(v/2);'
	db    'v.x*=u.y/u.z;'
	db    'gl_FragColor=vec4(j(v.xy,vec4(.1),76));'
	db '}'
	db 10,0
	
