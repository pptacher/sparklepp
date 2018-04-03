#version 410 core
in vec2 texcoord;
out vec4 color;

uniform sampler2D text;
uniform vec3 textColor;

void main(void) {
  vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, texcoord).r);
  color = vec4(textColor, 1.0) * sampled;
}
