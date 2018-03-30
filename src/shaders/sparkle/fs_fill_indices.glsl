#version 410 core

// fill a buffer with continuous indices, used in the sorting step.

in vec2 outTexcoord;

uniform uint width;
uniform uint height;

out uint indice;

void main(void) {

  indice = uint(floor(outTexcoord.x * width) + floor(outTexcoord.y * height) * width);

}
