#version 410 core

// fill a buffer with continuous indices, used in the sorting step.

#include "sparkle/interop.h"
in vec2 position;
in vec2 texcoord;

out vec2 outTexcoord;

void main() {
  outTexcoord = texcoord;
  gl_Position = vec4(position, 0.0f, 1.0f);
}
