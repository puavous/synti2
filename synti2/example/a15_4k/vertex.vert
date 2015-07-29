#version 120
// Vertex shader
uniform float s[200]; // State parameters from app.
uniform sampler2D tex; // One texture (learning as I do...)
varying vec3 normal;

float rndX(vec2 rndcent, vec2 rndoff, float rndscale){
    return texture2D(tex, rndcent+rndoff/rndscale).x;
}

float rndH(vec2 c, vec2 rndoff){
    return rndX(c,rndoff,2)*.2
        + rndX(c,rndoff,5)*.4
        + rndX(c,rndoff,10)*1
        + rndX(c,rndoff,25)*2;
}

void main(){
    vec4 v = gl_Vertex;

    // Take three heights and normal by cross prod:
    vec2 c = vec2(.001*s[0]);
    
    //float h1 = rndX(c,v.xy,5);
    float h1 = rndH(c,v.xy);
    float h2 = rndH(c,v.xy+vec2(.1,0));
    float h3 = rndH(c,v.xy+vec2(0,.1));

    // Then, map to xz plane and make height map:
    v.xz = v.xy;
    v.y = -4.;
    //v.xyz *= rndX(vec2(.02*s[0]),v.xy,5);
    //v.xyz *= 2.+sin(s[0]);
    v.z += -30+s[0];

    v.y += h1;

    normal = cross(vec3(.1,h2-h1,0),vec3(0,h3-h1,.1));
    normal = normalize(normal);

    v.xy = vec2(sin(s[0])*v.x+cos(s[0])*v.y, cos(s[0])*v.x-sin(s[0])*v.y);
    // Finally, project with the projection matrix given from app:
    gl_Position = gl_ProjectionMatrix * v;

    // "The old ftransform()" would be:
    //gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * v;
}
