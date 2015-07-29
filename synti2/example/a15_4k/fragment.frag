#version 120
uniform float s[200]; // State parameters from app.

uniform sampler2D tex; // One texture (learning as I do...)

varying vec3 normal;

vec2 rot(vec2 p, float th){
    float si = sin(th);
    float co = cos(th);
    return vec2(co*p.x - si*p.y, si*p.x + co * p.y);
}


float rndX(vec2 rndcent, vec2 rndoff, float rndscale){
    return texture2D(tex, rndcent+rndoff/rndscale).x;
}

void main(){
    // Normalize screen coordinates so that [-1,1]^2 becomes the
    // square in the middle; right and left sides extend over [-1,1],
    // depending on aspect ratio.
    vec2 spix = 1. - gl_FragCoord.xy / (.5 * vec2(s[1],s[2]));
    spix.x *= s[1]/s[2];
    vec2 pix = spix;

    // Add some distortions as time passes a bit:
    float th=0.01*s[0]*s[0];
    pix = rot(spix,th);
    pix.x *= 1.0+0.02*s[0]*(2.-2.*sin(s[0])*pix.y);

    vec2 rc = vec2(0.01*(s[0]-spix.y),.03*s[0]); // "fire-in-wind"
    //vec2 rc = vec2(0); pix += vec2(s[0]); // Stable "fire".

    // Random intensity:
    float inty = rndX(rc+vec2(0.), pix, 4)*.1
        + rndX(rc+vec2(.1), pix, 7)*.1
        + rndX(rc+vec2(.2), pix, 19)*.2
        + rndX(rc+vec2(.3), pix, 48)*.3
        + rndX(rc+vec2(.4), pix, 120)*.5;

    vec4 col = vec4(inty,inty*.3,.1,1.) / (1.+pow(length(spix.y-1),4.));

    col *= dot(normal,vec3(10,10,10));

    int iter = 30;

    gl_FragColor = col;

}
