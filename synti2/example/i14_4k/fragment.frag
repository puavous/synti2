uniform float s[200]; // State parameters from app.

// Structure for the ray march result.
struct marchRes {
  vec3 point;
  vec3 n;
  float closestD;
  float marchLen;
};

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


float f(vec3 p){
  if (sin(2.*s[0]) > .9){
    p.z -= 30.;
    return kissa(p);
  }else{
    p = rotZ(p,.3*sin(s[0])+sin(s[0])*p.z*.04);
    vec3 c = vec3(30.,30.,30.);
    p += vec3(0.,16.,60.*s[0]);

    vec3 q = mod(p,c)-0.5*c;
    return kissa( q );
  }
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

// Actual marching
marchRes march(vec3 from, vec3 direction) {
  float ThresholdDistance = .01;
  int MaxRaySteps = 180;
  float TooFar = 480.0;
  float totalDistance = 0.0;
  float closestDistance = TooFar;
  int steps;
  vec3 p;

  for (steps=0; steps < MaxRaySteps; steps++) {
    p = from + totalDistance * direction;
    float dist = f(p)*.9;
    totalDistance += dist;
    if (dist < ThresholdDistance){
      break;
    }
    if (dist < closestDistance){
      closestDistance = dist;
    }
    if (totalDistance > TooFar){
      steps = MaxRaySteps;
      break;
    }
  }
  return marchRes(p, normalEstimation(p),
                  closestDistance,
                  1.-float(steps)/float(MaxRaySteps));
}

// Phong model
vec4 doLightPhong(vec3 pcam, vec3 p, vec3 n, vec3 lpos,
                  vec3 lightC, vec3 amb, vec3 dfs, vec3 spec)
{
  // Ambient component:
  vec3 c = amb;

  // Compute light dir and dist; finished if light is behind the surface:
  vec3 ldir = normalize(lpos - p);
  float ldist = length(lpos-p);
  if (ldir.z > 0.) return vec4(c,1.);

  // Diffuse and specular component:
  float attn = 1. / (1. + 0.06*ldist + 0.006*ldist*ldist);
  vec3 camdir = normalize(p-pcam);
  vec3 idfs = dfs * max(dot(n,ldir),0.0);
  vec3 refldir = reflect(ldir, n);
  vec3 ispec = spec * pow(max(dot(refldir,camdir),0.0),2.0);
  c += attn * lightC * (idfs + ispec);
  return vec4 (c,1.); // no alpha blending in use...
}


void main(){
  // Expect screen width in s[1], height in s[2].
  // Compute normalized coordinate: x in [-ar,ar], y in [-1,1]
  // y increasing top to down. (eye at origin, facing positive z)
  vec2 pix = 1. - gl_FragCoord.xy / (.5 * vec2(s[1],s[2]));
  pix.x *= s[1]/s[2];

  vec3 cameraPosition = vec3(0.,0.,-40.);
  vec3 lightPosition = vec3(0.,5.,-40.);
  // I just shoot 'over there'.
  // TODO: Proper vector length and direction; from resol.
  vec3 vto = vec3(pix.x*2.,pix.y*2.,8.);
  vec3 vdir = normalize(vto - vec3(pix.x,pix.y,1.));

  marchRes pr = march(cameraPosition,vdir);
  vec3  p = pr.point;
  float r = pr.marchLen;
  vec3  n = pr.n;

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
