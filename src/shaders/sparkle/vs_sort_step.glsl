#version 410 core

in vec2 position;
in vec2 texcoord;
out vec2 outTexcoord;

void main(void)
{
  outTexcoord = texcoord;
  gl_Position = vec4(position, 0.0f, 1.0f);

}
