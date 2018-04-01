#version 410 core

// ============================================================================

/* Second Stage of the particle system :
 * - Update particle position and velocity,
 * - Apply curl noise,
 * - Apply vector field,
 * - Performs time integration,
 * - Handle collision detection,
 */

// ============================================================================

#include "sparkle/interop.h"
#include "sparkle/inc_curlnoise.glsl"

#define ENABLE_SCATTERING         0
#define ENABLE_VECTORFIELD        0
#define ENABLE_CURLNOISE          1

//time integration step.
uniform float uDeltaT;
//vector field sampler.
uniform sampler3D uVectorFieldSampler;
//simulation box dimension.
uniform float uBBoxSize;

in vec3 randvec;
in vec3 position;
in vec3 velocity;
in vec2 age;

out vec3 vsPosition;
out vec3 vsVelocity;
out vec2 vsAge;

TParticle PopParticle() {

  TParticle p;

  p.position = position;
  p.velocity = velocity;
  vec2 attribs = age;

  p.start_age = attribs.x;
  p.age = attribs.y;
  //p.id = floatBitsToUint(attribs.w);

  return p;
}

void PushParticle(in TParticle p) {

  vsPosition = p.position;
  vsVelocity = p.velocity;
  vsAge = vec2(p.start_age, p.age);

}

void UpdateParticle(inout TParticle p, in vec3 pos, in vec3 vel, in float age) {
  p.position.xyz = pos;
  p.velocity.xyz = vel;
  p.age = age;
}

float UpdateAge(in TParticle p) {
  float decay = 0.01*uDeltaT;
  float age = clamp(p.age - decay, 0.0f, p.start_age);
  return age;
}

vec3 ApplyForces() {
  vec3 force = vec3(0.0f);

#if ENABLE_SCATTERING
  //add a random force to each particles.
  const float scattering = 0.45f;
  vec3 randforce  = 2.0f * vec3(randvec.x, randvec.y, randvec.z) - 1.0f;
  force += scattering * randforce;
#endif

  return force;
}

vec3 ApplyRepulsion(in TParticle p) {
  vec3 push = vec3(0.0f);
/*
  //IDEA
  const vec3 vel = p.velocity.xyz;
  const vec3 pos = p.position.xyz;
  const float MAX_INFLUENCE_DISTANCE = 8.0f;

  vec3 n;
  float d = compute_gradient(pos, n);
  float coeff = smoothstep(0.0f, MAX_INFLUENCE_DISTANCE, abs(d));
  push = coeff * (n);
  //vec3 side = cross(cross(n, normalize(vel + vec3(1e-5))), n);
  //push = mix(push, side, coeff * coeff);
*/
  return push;
}

vec3 ApplyTargetMesh(in TParticle p) {
  vec3 pull = vec3(0.0f);

  /*
  //IDEA
  vec3 anchor = anchors_buffer[p.anchor_id];
  mat4x4 anchorModel = anchor_models_buffer[p.anchor_id];
  vec3 target = anchorModel * vec4(anchor, 1.0f);
  vec3 pull = target - p.position;
  float length_pull = length(pull);
  pull *= inversesqrt(length_pull);
  float factor = MAX_PULL_FACTOR * smoothstep(0.0f, MAX_PULL_FACTOR, length_pull);
  pull *= factor;
*/
  return pull;
}

vec3 ApplyVectorField(in TParticle p) {
  vec3 vfield = vec3(0.0f);

#if 1 //ENABLE_VECTORFIELD
  vec3 pt = p.position.xyz;

  ivec3 texsize = textureSize(uVectorFieldSampler, 0).xyz;
  vec3 extent = 0.5f * vec3(texsize.x, texsize.y, texsize.z);
  vec3 texcoord = (pt + extent) / (2.0f * extent);

  vfield = texture(uVectorFieldSampler, texcoord).xyz;

  //custom GL_CLAMP_TO_BORDER
  vec3 clamp_to_border = step(-extent, pt) * step(pt, +extent);
  bool b = any(lessThan(clamp_to_border, vec3(1.0f)));
  vfield = mix(vfield, vec3(0.0f), float(b));
#endif

  return vfield;
}

vec3 GetCurlNoise(in TParticle p) {
  vec3 curl = vec3(0.0f);

#if ENABLE_CURLNOISE
  const float effect = 1.0f;
  const float scale = 1.0f / 128.0f;
  curl  = effect * compute_curl(p.position.xyz * scale);
#endif

  return curl;
}


// ----------------------------------------------------------------------------

void CollideSphere(float r, in vec3 center, inout vec3 pos, inout vec3 vel) {
  vec3 p = pos - center;

  float dp = dot(p, p);
  float r2 = r*r;

  if (dp > r2) {
    vec3 n = -p * inversesqrt(dp);
    vel = reflect(vel, n);

    pos = center - r*n;
  }
}

void CollideBox(in vec3 corner, in vec3 center, inout vec3 pos, inout vec3 vel) {
  vec3 p = pos - center;

  if (p.x < -corner.x) {
    p.x = -corner.x;
    vel = reflect(vel, vec3(1.0f,0.0f,0.0f));
  }

  if (p.x > corner.x) {
    p.x = corner.x;
    vel = reflect(vel, vec3(-1.0f,0.0f,0.0f));
  }

  if (p.y < -corner.y) {
    p.y = -corner.y;
    vel = reflect(vel, vec3(0.0f,1.0f,0.0f));
  }

  if (p.y > corner.y) {
    p.y = corner.y;
    vel = reflect(vel, vec3(0.0f,-1.0f,0.0f));
  }

  if (p.z < -corner.z) {
    p.z = -corner.z;
    vel = reflect(vel, vec3(0.0f,0.0f,1.0f));
  }

  if (p.z > corner.z) {
    p.z = corner.z;
    vel = reflect(vel, vec3(0.0f,0.0f,-1.0f));
  }

  pos = p + center;
}

void CollisionHandling(inout vec3 pos, inout vec3 vel) {
  float r = 0.5f * uBBoxSize;

  CollideSphere(r, vec3(0.0f), pos, vel);
  CollideBox(vec3(r), vec3(0.0f), pos, vel);

}

void main() {
  vec3 dt = vec3(uDeltaT);

  //local copy of the particle.
  TParticle p = PopParticle();

  //update age.
  /// [ dead particles still have a positive age, but are not push in render buffer.
  ///   Still set their age to zero ? ]
  float age = UpdateAge(p);

  if (age > 0.0f) {
    //apply external forces.
    vec3 force = ApplyForces();

    //apply repulsion on simple objects.
    //force += ApplyRepulsion(p);

    //apply mesh targeting.
    //force += ApplyTargetMesh(p);

    //apply vector field.
    //force += ApplyVectorField(p);

    //integrate velocity.
    vec3 vel = fma(force, dt, p.velocity.xyz);

    //get curling noise.
    vec3 noise_vel =  ApplyVectorField(p);

    vel += noise_vel;
    noise_vel *= 0.0f;
    vel = 10*normalize(vel);

    //integrate position.
    vec3 pos = fma(vel  + noise_vel, dt, p.position.xyz);

    //handle collision.
    CollisionHandling(pos, vel);

    //update particle.
    UpdateParticle(p, pos, vel, age);
    PushParticle(p);
  }
}
