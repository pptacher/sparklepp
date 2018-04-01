#include "api/vector_field.h"

#include <cfloat>
#include <cmath>
#include <cstdio>
#include <vector>

#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/noise.hpp"
#include <boost/progress.hpp>
#include "noise.h"

using namespace glm;

static const vec3 sphereCenter(0, 0, 0);
static const float sphereRadius = 1.0f;
static const float epsilon = 1e-10f;
static const float noise_length_scale[] = {0.4f, 0.23f, 0.11f};
static const float noise_gain[] = {1.0f, 0.5f, 0.25f};
static const float plumeCeiling(3);
static const float plumeBase(-3);
static const float plumeHeight(80);
static const float ringRadius(10.25f);
static const float ringSpeed(0.3f);
static const float ringsPerSecond(0.125f);
static const float ringMagnitude(10);
static const float ringFalloff(0.7f);
static const float particlesPerSecond(64000);
static const float seedRadius(0.125f);
static const float initialBand(0.1f);
static FlowNoise3  noise;

inline float noise0(vec3 s) { return noise(s.x, s.y, s.z); }
inline float noise1(vec3 s) { return noise(s.y + 31.416f, s.z - 47.853f, s.x + 12.793f); }
inline float noise2(vec3 s) { return noise(s.z - 233.145f, s.x - 113.408f, s.y - 185.31f); }
inline vec3 noise3d(vec3 s) { return vec3(noise0(s), noise1(s), noise2(s)); };
inline vec3 blendVectors(vec3 potential, float alpha, vec3 gradient)
                            {return alpha * potential + (1 - alpha) * dot(potential, gradient) * gradient;}

void VectorField::initialize(unsigned int const width, unsigned int const height, unsigned int const depth) {
  dimensions_ = glm::vec3(width, height, depth);

  // @bug
  /// Velocity fields are 3d textures where only particles in the texture volume
  /// are affected. Therefore particles outside the volume should be clamped to
  /// zeroes when the texture is sampled.
  /// However there is some strange and buggy behavior when specifying a border
  /// of 3 or more values, so here only two are specified for the 3d texture.
  /// Wrap mode is set to clamp_to_edge for now.

  GLint const filter_mode = GL_LINEAR;
  GLint const wrap_mode = GL_CLAMP_TO_EDGE;
  //GLfloat const border[4u] = {0.0f, 0.0f, 0.0f, 0.0f};

  glGenTextures(1u, &gl_texture_id_);
  glBindTexture(GL_TEXTURE_3D, gl_texture_id_);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, filter_mode);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, filter_mode);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, wrap_mode);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, wrap_mode);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, wrap_mode);
    //glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, border);
  glBindTexture(GL_TEXTURE_3D, 0u);

  CHECKGLERROR();
}

void VectorField::deinitialize() {
  glDeleteTextures(1u, &gl_texture_id_);
}

void VectorField::generate_values(char const *filename) {
  unsigned int const W = dimensions_.x;
  unsigned int const H = dimensions_.y;
  unsigned int const D = dimensions_.z;
  //float const dW = 1.0f / static_cast<float>(W);
  //float const dH = 1.0f / static_cast<float>(H);
  //float const dD = 1.0f / static_cast<float>(D);

  std::vector<float> data(3u*W*H*D);

  //if true the data will be recalculated.
  bool bForceCalculate = false;

  //read data form a file if it exists.
  FILE *fd = fopen(filename, "rb");

  if (fd) {
    size_t nreads = fread(&data[0u], sizeof(float), data.size(), fd);
    if (nreads != data.size()) {
      fprintf(stderr, "Velocity field : incorrect velocity file \"%s\", recalculating.\n", filename);
      bForceCalculate = true;
    }
  }
  boost::progress_display progress(W*H*D);
  //or recalculate the data.
  if (!fd || bForceCalculate) {
    std::vector<float>::iterator pData = data.begin();
    for (size_t z = 0; z < D; z++) {
      //float const dz = z * dD;
      for (size_t y = 0; y < H; y++) {
        //float const dy = y * dH;
        for (size_t x = 0; x < W ; x++) {
          //float const dx = x * dW;
          //unsigned int const index = 3u * z * (height_ * width_) + (y * width_) + x;
          vec3 p;
          p[0] = - (float)W + 2.0f * x;
          p[1] = -(float)H + 2.0f * y;
          p[2] = -(float)D + 2.0f * z;

          glm::vec3 v = get_curl_noise(p);

          *pData++ = v.x;//std::cout << v.x << '\n';
          *pData++ = v.y;//std::cout << v.y << '\n';
          *pData++ = v.z;//std::cout << v.z << '\n';
          ++progress;
        }
      }
    }

    //save on disk.
    fd = fopen(filename, "wb");
    fwrite(&data[0u], sizeof(float), data.size(), fd);
  }
  fclose(fd);

  //transfer pixels to device.
  glBindTexture(GL_TEXTURE_3D, gl_texture_id_);
  glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGB32F, W, H, D);
  glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, W, H, D, GL_RGB, GL_FLOAT, data.data());
  glBindTexture(GL_TEXTURE_3D, 0u);

  CHECKGLERROR();
}

vec3 VectorField::compute_curl(vec3 p) {

  const float e = 1e-4f;
  vec3 dx(e, 0, 0);
  vec3 dy(0, e, 0);
  vec3 dz(0, 0, e);

  float x = sample_potential(p+dy)[2] - sample_potential(p-dy)[2]
            - sample_potential(p + dz)[1] + sample_potential(p - dz)[1];

  float y = sample_potential(p + dz)[0] - sample_potential(p - dz)[0]
            - sample_potential(p + dx)[2] + sample_potential(p - dx)[2];

  float z = sample_potential(p + dx)[1] - sample_potential(p - dx)[1]
            - sample_potential(p + dy)[0] + sample_potential(p - dy)[0];

  return vec3(x, y, z) / (2*e);
}

vec3 VectorField::get_curl_noise(vec3 p) {

  const float effect = 1.0f;
  const float scale = 1.0f / 128.0f;
  return effect * compute_curl(p * scale);

}

vec3 VectorField::sample_potential (vec3 p) {
  vec3 psi(0, 0, 0);
  vec3 gradient  = compute_gradient(p);

  //std::cout << "gradient: " << gradient.x << gradient.y << '\n';

  float obstacle_distance = sample_distance(p);

  // add turbulence octaves that respect boundaries, increasing upwards
  float height_factor = 1.0f;//ramp((p.y - plume_base) / plume_height);
  for (size_t i = 0; i < countof(noise_length_scale); i++) {
    vec3 s = vec3(p) / noise_length_scale[i];
    float d = ramp(std::fabs(obstacle_distance) / noise_length_scale[i]);
    //std::cout << "d: " << d << '\n';
    vec3 psi_i = blendVectors(noise3d(s), d, gradient);
    psi += height_factor * noise_gain[i] * psi_i;
    //std::cout << "psi: " << psi.x << psi.y << psi.z << '\n';
  }

  /*vec3 risingForce = vec3(0, 0, 0) - p;
  risingForce = vec3(-risingForce[2], 0, risingForce[0]);

  //add rising vortex rings.
  float ring_y = plumeCeiling;
  float d =ramp(std::fabs(obstacle_distance) / ringRadius);
  while (ring_y > plumeBase)  {
    float ry = p.y - ring_y;
    float rr = std::sqrt(p.x*p.x + p.z*p.z);
    float rmag = ringMagnitude / (sqr(rr-ringRadius)+sqr(rr+ringRadius)+sqr(ry)+ringFalloff);
    vec3 rpsi = rmag * risingForce;
    psi += blendVectors(rpsi, d, gradient);
    ring_y -= ringSpeed / ringsPerSecond;
  }*/

  return psi;
}

vec3 VectorField::compute_gradient(vec3 p) {
  const float e = 0.01f;
  vec3 dx(e, 0.0f, 0.0f);
  vec3 dy(0.0f, e, 0.0f);
  vec3 dz(0.0f, 0.0f, e);

  float d = sample_distance(p);
  float dfdx = sample_distance(p + dx) - d;
  float dfdy = sample_distance(p + dy) - d;
  float dfdz = sample_distance(p + dz) - d;
  //std::cout << "dfdy: " << dfdy << '\n';

  return normalize(vec3(dfdx, dfdy, dfdz));
  //return vec3(dfdx, dfdy, dfdz);
}

float VectorField::sample_distance(vec3 p) {
//std::cout << "p.y" << p.y << '\n';
  return p.y;

  //vec3 u = p - sphereCenter;
  //float d = length(u);
  //return d - sphereRadius;
}
