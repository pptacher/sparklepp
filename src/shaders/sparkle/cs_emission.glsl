#version 410 core

// ============================================================================
/*
 * First Stage of the particle system.
 * Initialize particles and add them to a buffer
*/
// ============================================================================

#include "sparkle/interop.h"
#include "sparkle/inc_math.glsl"

uniform uint uEmitCount = 0u;
uniform vec3 uEmitterPosition = vec3(0.0f, 0.0f, 0.0f);
uniform vec3 uEmitterDirection = vec3(1.0f, 0.0f, 1.0f);
uniform float uParticleMaxAge;

in vec3 randvec;

out vec3 tfPosition;
out vec3 tfVelocity;
out vec2 tfAge;

void PushParticle(in vec3 position, in vec3 velocity, in float age) {

  tfPosition = position;
  tfVelocity = velocity;
  tfAge = vec2(age, age);

}

float rand(vec2 co){
  return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void CreateParticle(const uint gid) {

  vec3 rn = randvec * rand(vec2(1.0f, floor(gid / PARTICLES_KERNEL_GROUP_WIDTH)));
  vec3 pos = uEmitterPosition;
  vec3 vel = uEmitterDirection * rand(vec2(1.0f, floor(gid / PARTICLES_KERNEL_GROUP_WIDTH)));

  float r = 60.0f * rn.x;
  vec3 theta = rn * TwoPi();

#if 0
  //position in a sphere with omnidirectional velocity.
  pos = r * uEmitterDirection * rotationZ(theta.z) * rotationY(theta.y);
  //vel = normalize(pos);
#else
  //position on a plate going up.
  pos.x = r * cos(theta.y);
  pos.y = 0.1f * rn.z;
  pos.z = r * sin(theta.y);
  //vel = vec3(0.0f);
#endif

  //the age is set by group to assure to have a number of particles factor of groupWidth.
  // do a gl_VertexID modulo PARTICLES_KERNEL_GROUP_WIDTH to get the group ID.
  float age = uParticleMaxAge * mix(0.2f, 1.0f, rand(vec2(1.0f, floor(gid / PARTICLES_KERNEL_GROUP_WIDTH))));
  //float age = uParticleMaxAge * mix(0.2f, 1.0f, rand(randvec.xy));

  PushParticle(pos, vel, age);
}

void main() {
  uint gid = gl_VertexID;

  if (gid < uEmitCount) {
    CreateParticle(gid);
  }
}
