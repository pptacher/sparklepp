// ============================================================================

/**
 * This file is shared between Host and Device code for interoperability
 * and assert structural integrity.
 */

// ============================================================================

#ifndef SHADERS_SPARKLE_INTEROP_H_
#define SHADERS_SPARKLE_INTEROP_H_

// ----------------------------------------------------------------------------

// Kernel group width used across the particles pipeline.
#define PARTICLES_KERNEL_GROUP_WIDTH      256u

struct TParticle {
  vec3 position;
  vec3 velocity;
  float start_age;
  float age;
//  float _padding0;
//  uint id;
};

#undef SHADER_UINT

// ----------------------------------------------------------------------------

#endif  // SHADERS_SPARKLE_INTEROP_H_
