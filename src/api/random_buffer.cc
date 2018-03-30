#include "api/random_buffer.h"

#include "linmath.h"
#include "shaders/sparkle/interop.h"

void RandomBuffer::initialize(unsigned int const nelements) {
  num_elements_ = nelements;

  glGenBuffers(1u, &random_buffer_id_);
  glBindBuffer(GL_ARRAY_BUFFER, random_buffer_id_);
    glBufferData(GL_ARRAY_BUFFER, num_elements_ * sizeof(GLfloat), nullptr, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0u);

  CHECKGLERROR();
}

void RandomBuffer::deinitialize() {
  glDeleteBuffers(1u, &random_buffer_id_);
}

void RandomBuffer::generate_values() {
  std::uniform_real_distribution<float> distrib(min_value_, max_value_);

  glBindBuffer(GL_ARRAY_BUFFER, random_buffer_id_);
  //seems heavy ?
  float *buffer = (float*)glMapBufferRange(
    GL_ARRAY_BUFFER, 0u, num_elements_ * sizeof(float),
    GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

  for (size_t i = 0; i < num_elements_; i++) {
    buffer[i] = distrib(mt_);
  }

  glUnmapBuffer(GL_ARRAY_BUFFER);
  glBindBuffer(GL_ARRAY_BUFFER, 0u);

  CHECKGLERROR();
}

void RandomBuffer::bind() {
  glBindBuffer(GL_ARRAY_BUFFER, random_buffer_id_);
}

void RandomBuffer::unbind() {
  glBindBuffer(GL_ARRAY_BUFFER, 0u);
}
