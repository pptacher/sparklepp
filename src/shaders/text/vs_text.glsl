#version 410 core
layout (location = 0) in vec4 vertex;
out vec2 texcoord;

uniform mat4 projection;

void main(void) {
  gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
  texcoord = vertex.zw;
}
