#ifndef API_APPEND_CONSUME_BUFFER_H_
#define API_APPEND_CONSUME_BUFFER_H_

#include "opengl.h"

class AppendConsumeBuffer {
public:
  AppendConsumeBuffer( unsigned int const element_count, unsigned int const attrib_buffer_count)
    : element_count_(element_count),
    attrib_buffer_count_(attrib_buffer_count),
    storage_buffer_size_(element_count * 8 * sizeof(GLfloat)),
    array_buffer_ids_{0u, 0u}
    {}

  void initialize();
  void deinitialize();

  void bind();
  void unbind();

  void bind_attributes();
  void unbind_attributes();

  void swap_storage();

  unsigned int element_count() const { return element_count_;}
  unsigned int storage_buffer_size() const { return storage_buffer_size_; }

  GLuint first_array_buffer_id() const { return array_buffer_ids_[0u]; }
  GLuint second_array_buffer_id() const { return array_buffer_ids_[1u]; }

private:
  unsigned int const element_count_;              //number of elements in one buffer.

  unsigned int const attrib_buffer_count_;
  unsigned int const storage_buffer_size_;        //one buffer bytesize

  GLuint array_buffer_ids_[2u];                     //array buffer (append and consume)
};

#endif //API_APPEND_CONSUME_BUFFER_H_
