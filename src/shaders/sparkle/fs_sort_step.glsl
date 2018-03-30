#version 410 core

in vec2 outTexcoord;

uniform uint uMaxBlockWidth;
uniform uint uBlockWidth;
uniform uint width;
uniform uint height;
uniform sampler2D dp;
uniform usampler2D indices;

out uint color;

void main(void) {

  vec2 ownPosition = outTexcoord;
  //does not compile when using const qualifier.
  uint max_block_width = uMaxBlockWidth;
  uint block_width = uBlockWidth;
  uint pair_distance = block_width / 2u;

  // get self
  uvec4 val0 =  texture(indices, ownPosition);
  uint i = uint(floor(ownPosition.x * width) + floor(ownPosition.y * height) * width);

  int direction =  ((i % uBlockWidth) < pair_distance) ? 1: -1; // which half in the block ?
  int orientation = ((i / max_block_width ) % 2 == 0) ? 1 : -1; // orientation of the comparison arrow in the block.

  uint j = i + direction * pair_distance;

  uvec4 val1 = texture(indices, vec2( float((j % width)) / float(width), float(j) / (float(width * height )) ));

  int order = direction * orientation;

  float dp0 = (texture(dp, vec2(float((val0.x % width)) / float(width), float(val0.x) / (float(width * height ))) ) ).x;
  float dp1 = (texture(dp, vec2(float((val1.x % width)) / float(width), float(val1.x) / (float(width * height ))) ) ).x;
  color = ( order*dp0 > order*dp1 ) ? val0.x : val1.x ;

}
