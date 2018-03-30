#version 410 core

// ============================================================================

/* Second Stage of the particle system :
 * - filter dead particles
 */

// ============================================================================
in vec3 vsPosition[1];
in vec2 vsAge[1];
in vec3 vsVelocity[1];

layout(points) in;
layout(points, max_vertices = 1) out;

out vec3 tfPosition;
out vec3 tfVelocity;
out vec2 tfAge;

void main(void) {

  if ( vsAge[0].y > 0) {
    tfPosition = vsPosition[0];
    tfVelocity = vsVelocity[0];
    tfAge = vsAge[0];

    EmitVertex();
    EndPrimitive();
  }

}
