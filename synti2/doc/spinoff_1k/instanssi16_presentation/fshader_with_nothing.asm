minimized_fragment_shader_str:
 	db '#version 120',10
	db 'uniform vec4 u;'
	db 'vec2 t(vec2 v){'
	db    'float p=u.x*sin(mod(u.x,1))*v.x*sin(mod(u.x,.6))*v.y;'	
	db    'return (.6+v/2)*dot(v-p,v*p);'
	db '}'
	db 'void main(){'
	db    'vec2 v=u.yz;'
	db    'v=1-gl_FragCoord.xy/(v/2);'
	db    'v.x*=u.y/u.z;'
	db    'v=vec2(sin(u.x)*v.x+cos(u.x)*v.y,cos(u.x)*v.x-sin(u.x)*v.x);'
	db    'v.xy=t(v);'
	db    'gl_FragColor=vec4(.6,v,1);'
	db '}'
	db 10,0
	
