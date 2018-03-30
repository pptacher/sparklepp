#include "api/append_consume_buffer.h"

#include "linmath.h"
#include "shaders/sparkle/interop.h"

#include <iostream>

namespace {
  void SwapUint(unsigned int &a, unsigned int &b) {
    a ^= b;
    b ^= a;
    a ^= b;
  }
}

void AppendConsumeBuffer::initialize() {
  //storage buffers.
  glGenBuffers(2u, array_buffer_ids_);

  glBindBuffer(GL_ARRAY_BUFFER, array_buffer_ids_[0u]);
    glBufferData(GL_ARRAY_BUFFER, storage_buffer_size_, nullptr, GL_STREAM_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, array_buffer_ids_[1u]);
    glBufferData(GL_ARRAY_BUFFER, storage_buffer_size_, nullptr, GL_STREAM_DRAW);

  //unbind buffers.
  glBindBuffer(GL_ARRAY_BUFFER, 0u);

  CHECKGLERROR();
}

void AppendConsumeBuffer::deinitialize() {
  glDeleteBuffers(2u, array_buffer_ids_);

  CHECKGLERROR();
}

void AppendConsumeBuffer::bind() {
  bind_attributes();
}

void AppendConsumeBuffer::unbind() {
  unbind_attributes();
}

void AppendConsumeBuffer::bind_attributes() {

}

void AppendConsumeBuffer::unbind_attributes() {

}

void AppendConsumeBuffer::swap_storage() {
#if 0
  //costly but safe.
  //glCopyBufferSubData(...?
#else
  // Cheapest alternative but can makes some particle flicks
  // due to non updated rendering params (workaround possible).
  SwapUint(array_buffer_ids_[0u], array_buffer_ids_[1u]);
#endif
}
