uniform float s[2000]; // State parameters from app.
/*
// FIXME: Should do proper rotation matrices?
// (my axes can be the wrong way around..
// I made this 'out from my head')
vec3 rotY(vec3 p, float th){
  float si = sin(th);
  float co = cos(th);
  return vec3(si*p.x - co*p.z, p.y, co*p.x + si * p.z);
}

vec3 rotZ(vec3 p, float th){
  float si = sin(th);
  float co = cos(th);
  return vec3(si*p.x - co*p.y, co*p.x + si * p.y, p.z);
}

vec3 rotX(vec3 p, float th){
  float si = sin(th);
  float co = cos(th);
  return vec3(p.x, si*p.y - co*p.z, co*p.y + si * p.z);
}

float sdTorus( vec3 p, vec2 t )
{
  vec2 q = vec2(length(p.xy)-t.x,p.z);
  return length(q)-t.y;
}

// A heart-shape - just a bent torus
float heart(vec3 p, vec2 wt)
{
  p.y -= abs(p.x);
  return sdTorus(p, vec2(wt.x,wt.y));
}

float h(vec3 p){
  p = rotZ(p,s[0]*.1);
  p = rotX(p,s[0]*.2);
  p = rotY(p,s[0]);
  float f1 = heart(p+vec3(4.,0.,0.), vec2(5.,1.));
  p = rotX(p,3.14+sin(s[0]*.2));
  float f2 = heart(p, vec2(5.+s[6]*2.,1.));
  return min(f1,f2);
}

float f_intro(vec3 p){
  p = rotY(p,s[0]);
  float f1 = heart(p, vec2(7.+s[6]*2.,1.));
  return f1;
}

float f(vec3 p){
  // My synth almost supports full synchronization of everything in the
  // animation, but this production still needs hard-coded time sync.
  // This is very unfortunate, but I'll make it better before my next
  // demoscene entry...
  if (s[0]<29.6){
    return f_intro(p);
  }else{
    return h(p);
  }
}




  void main(){
    vec3 cameraPosition = vec3(0.,0.,-20.);
    vec3 lightPosition = vec3(-10.);

    // I just shoot 'over there'.
    // TODO: Proper vector length and direction; from resol.
    vec2 pix = gl_FragCoord.xy / vec2(s[1]/2.,s[2]/2.) - vec2(1.,1.);
    pix.x *= s[1]/s[2];
    vec3 vdir = vec3(pix.x,pix.y,1.);
    // Hmm.. think about how the direction affects the rendering:
    vdir = normalize(vdir) * .5;

    vec4 pr = march(cameraPosition,vdir);
    vec3 p = pr.xyz;
    float r = pr.w;
    vec3 n = normalEstimation(p);

    // White dot in the center, for debugging:
    // FIXME: Don't leave this in final version, pls.
    //    if (length(pix) < 0.01){
    //      gl_FragColor = vec4(1.0,1.0,1.0,0.5);
    //      return;
    //    }
    // Lighting computation.
    if (r>0.0){
      vec3 lightC = vec3(3.);
      vec3 ambient = vec3(0.3,0.0,0.0);
      vec3 diffuse = vec3(1.0,0.0,0.0);
      vec3 specular = vec3(1.0,1.0,0.0);

      vec4 color = doLightPhong(cameraPosition, p, n,
                             lightPosition,
                             lightC,ambient,diffuse,specular);

      float darken = 1.0-max((distance(cameraPosition,p)/120.0),0.0);
      gl_FragColor = color * darken * r;
    } else {
      // Could have some action in the background:
      if (s[0]>15.){
        vec3 bgp = rotZ(vec3(pix,0.),.2*s[0]);
        gl_FragColor = vec4(sin(s[0])*cos(4.*bgp.x*bgp.y)+s[3]);
      } else {
        gl_FragColor = vec4(0.);
      }
    }
    // Fade out at the end:
    gl_FragColor*=1.-smoothstep(56.,65.,s[0]);
  }
*/

  /* Pretty much the simplest possible visualization. Squares'
   * intensities show the values in the synthesizer state.
   */

float sdSphere( vec3 p, float s )
{
  return length(p)-s;
}

float kissa(vec3 p){
  return sdSphere(p, 5.);
}


float f(vec3 p){
  return kissa(p);
}


vec4 march(vec3 from, vec3 direction) {
  float MinimumDistance = .01;
  int MaxRaySteps = 180;
  float TooFar = 80.0; // shader_minifier got confused if this was outside.. 
  float totalDistance = 0.0;
  int steps;
  vec3 p;

  for (steps=0; steps < MaxRaySteps; steps++) {
    p = from + totalDistance * direction;
    float distance = f(p)*.9;
    totalDistance += distance;
    if (distance < MinimumDistance){
      break;
    }
    if (totalDistance > TooFar) return vec4(p,0.0);
  }
  vec4 res; res.xyz = p;
  res.w = 1.-float(steps)/float(MaxRaySteps);
  return res;
}


// Distance estimation describes a hyperplane (local tangent plane of
// a geometry) in an implicit form (f(p)==0 exactly on the plane,
// f(p)>0 on the front side). Gradient of the function gives a vector
// orthogonal to the plane. Here we approximate the gradient with finite
// differences along the coordinate axes.

const float epsilon = 0.1;

vec3 normalEstimation(vec3 p){
  return normalize( vec3(
      f(p + vec3(epsilon,0.,0.) ) - f(p - vec3(epsilon,0.,0.))
    , f(p + vec3(0.,epsilon,0.) ) - f(p - vec3(0.,epsilon,0.))
    , f(p + vec3(0.,0.,epsilon) ) - f(p - vec3(0.,0.,epsilon))
) );}

// Phong model
vec4 doLightPhong(vec3 pcam, vec3 p, vec3 n, vec3 lpos,
                  vec3 lightC, vec3 amb, vec3 dfs, vec3 spec)
{
  // Ambient component:
  vec3 c = amb;

  // Compute light direction; finished if light is behind the surface:
  vec3 ldir = normalize(lpos - p);
  if (ldir.z > 0.) return vec4(c,1.);

  // Diffuse and specular component:
  float ldist = length(lpos-p);
  float attn = 1. / (1. + 0.03*ldist + 0.003*ldist*ldist);
  vec3 camdir = normalize(p-pcam);
  vec3 idfs = dfs * max(dot(n,ldir),0.0);
  vec3 refldir = reflect(ldir, n);
  vec3 ispec = spec * pow(max(dot(refldir,camdir),0.0),4.0);
  c += attn * lightC * (idfs + ispec);
  return vec4 (c,1.); // no alpha blending in use...
}


  void main(){
    vec2 pix = 1. - gl_FragCoord.xy / (.5 * vec2(s[1],s[2]));
    pix.x *= s[1]/s[2];

    vec3 cameraPosition = vec3(0.,0.,-20.);
    vec3 lightPosition = vec3(-10.);
    // I just shoot 'over there'.
    // TODO: Proper vector length and direction; from resol.
    vec3 vdir = vec3(pix.x,pix.y,1.);
    // Hmm.. think about how the direction affects the rendering:
    vdir = normalize(vdir);

    vec4 pr = march(cameraPosition,vdir);
    vec3 p = pr.xyz;
    float r = pr.w;
    vec3 n = normalEstimation(p);

    vec3 lightC = vec3(3.);
    vec3 ambient = vec3(0.3,0.3,0.4);
    vec3 diffuse = vec3(0.8,0.0,0.4);
    vec3 specular = vec3(1.0,1.0,1.0);

    vec4 color = doLightPhong(cameraPosition, p, n,
                              lightPosition,
                              lightC,ambient,diffuse,specular);
    
    gl_FragColor = color;
  }

