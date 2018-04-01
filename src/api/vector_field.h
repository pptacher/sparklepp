#ifndef API_VECTOR_FIELD_H_
#define API_VECTOR_FIELD_H_

#include "opengl.h"
#include "glm/glm.hpp"

class VectorField {
public:
  VectorField()
    : gl_texture_id_(0u)
  {}

  void initialize(unsigned int const, unsigned int const, unsigned int const);
  void deinitialize();

  //generate vector datas.
  //load them from filename if it exists, otherwise compute them and save on disk.
  void generate_values(char const *filename);
  glm::vec3 compute_curl(glm::vec3);
  glm::vec3 compute_gradient(glm::vec3);
  glm::vec3 get_curl_noise(glm::vec3);
  float sample_distance(glm::vec3);
  glm::vec3 sample_potential(glm::vec3);

  inline const glm::uvec3& dimensions() const {
    return dimensions_;
  }

  inline const glm::vec3& position() const {
    return position_;
  }

  inline GLuint texture_id() const {
    return gl_texture_id_;
  }

private:

  glm::uvec3 dimensions_;
  glm::vec3 position_;
  GLuint gl_texture_id_;
};


#endif
