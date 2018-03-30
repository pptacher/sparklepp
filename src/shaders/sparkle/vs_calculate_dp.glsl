#version 410 core

/*
 * Compute in view space the dot product between particles position and the
 * view vector, giving their distance to the camera.
 *
 * Used to sort particles for alpha-blending before rendering.
*/

#include "sparkle/interop.h"

uniform mat4 uViewMatrix;

in vec3 position;
in vec3 velocity;
in vec2 age;

out float tfDp;

void main() {

  vec4 positionVS = uViewMatrix * vec4(position, 1.0f);

  //the default front of camera in view space.
  vec3 targetVS = vec3(0.0f, 0.0f, -1.0f);

  //distance of the particle from the camera.
  tfDp = dot(targetVS, positionVS.xyz);
}
