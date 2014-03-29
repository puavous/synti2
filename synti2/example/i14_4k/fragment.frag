uniform float s[200]; // State parameters from app.

vec3 rotY(vec3 p, float th){
  float si = sin(th);
  float co = cos(th);
  return vec3(si*p.x - co*p.z, p.y, co*p.x + si * p.z);
}

vec3 rotZ(vec3 p, float th){
  float si = sin(th);
  float co = cos(th);
  return vec3(co*p.x - si*p.y, si*p.x + co * p.y, p.z);
}

vec3 rotX(vec3 p, float th){
  float si = sin(th);
  float co = cos(th);
  return vec3(p.x, si*p.y - co*p.z, co*p.y + si * p.z);
}

float sdBox( vec3 p, vec3 b )
{
  vec3 d = abs(p) - b;
  return min(max(d.x,max(d.y,d.z)),0.0) +
         length(max(d,0.0));
}

float sdSphere( vec3 p, float s )
{
  return length(p) - s;
}

float kissa(vec3 p){
  p.y -= 4.;
  vec3 q = p + vec3(0.,9.,0.);
  float korvat = sdBox(q, vec3(4.5,2.,1.));
  float maha = sdSphere(p, 7.);
  p.y += 7.;
  float paa = sdSphere(p, 5.);
  return min(maha,min(paa,korvat));
}

/*
float catField( vec3 p, vec3 c )
{
    vec3 q = mod(p,c)-0.5*c;
    return kissa( q );
}

float one(vec3 p){
    return sdBox(p, vec3(1.,4.,1.));
}

float two(vec3 p){
  float a = sdBox(p + vec3(-2.,0.,0.), 
                  vec3(1.,4.,1.));

  float b = sdBox(p + vec3(2.,0.,0.), 
                  vec3(1.,4.,1.));
  return min(a,b);
}

float three(vec3 p){
  float a = sdBox(p + vec3(-4.,0.,0.), 
                  vec3(1.,4.,1.));
  float b = sdBox(p + vec3(0.,0.,0.), 
                  vec3(1.,4.,1.));
  float c = sdBox(p + vec3(4.,0.,0.), 
                  vec3(1.,4.,1.));
  return min(a,min(b,c)); 
}

*/

float f(vec3 p){
  /*
  if (sin(2.*s[0]) > .9){
    return one(p);
  } else if (sin(2.*s[0]) > .8){
    return two(p);
  } else if (sin(2.*s[0]) > .7){
    return three(p);
  } else if (sin(2.*s[0]) > .6){
  */
    p.z -= 30.;
    return kissa(p);
    /*
  } else {
    p = rotZ(p,.3*sin(s[0])+sin(s[0])*p.z*.04);
    return catField(p+vec3(0.,16.,60.*s[0]),
                    vec3(30.,30.,30.));
  }
    */
}


vec4 march(vec3 from, vec3 direction) {
  float MinimumDistance = .01;
  int MaxRaySteps = 180;
  float TooFar = 480.0;
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

vec3 normalEstimation(vec3 p){
  float epsilon = 0.1;
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
  float attn = 1. / (1. + 0.06*ldist + 0.006*ldist*ldist);
  vec3 camdir = normalize(p-pcam);
  vec3 idfs = dfs * max(dot(n,ldir),0.0);
  vec3 refldir = reflect(ldir, n);
  vec3 ispec = spec * pow(max(dot(refldir,camdir),0.0),2.0);
  c += attn * lightC * (idfs + ispec);
  return vec4 (c,1.); // no alpha blending in use...
}


  void main(){
    vec2 pix = 1. - gl_FragCoord.xy / (.5 * vec2(s[1],s[2]));
    pix.x *= s[1]/s[2];

    vec3 cameraPosition = vec3(0.,0.,-40.);
    vec3 lightPosition = vec3(0.,5.,-40.);
    // I just shoot 'over there'.
    // TODO: Proper vector length and direction; from resol.
    vec3 vto = vec3(pix.x*2.,pix.y*2.,8.);
    vec3 vdir = vto - vec3(pix.x,pix.y,1.);
    // Hmm.. think about how the direction affects the rendering:
    vdir = normalize(vdir);

    vec4 pr = march(cameraPosition,vdir);
    vec3 p = pr.xyz;
    float r = pr.w;
    vec3 n = normalEstimation(p);

    vec3 lightC = vec3(8.);
    vec3 ambient = vec3(0.2,0.0,0.0);
    vec3 diffuse = vec3(0.8,0.8,0.8);
    vec3 specular = vec3(0.8,0.8,1.0);

    vec4 color = doLightPhong(cameraPosition, p, n,
                              lightPosition,
                              lightC,ambient,diffuse,specular);

    color *= (300.-p.z)/300.;
    
    if (r<.01) color = vec4(0.);
    gl_FragColor = 1.5*color;
  }
