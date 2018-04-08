#ifndef API_GPU_PARTICLE_H
#define API_GPU_PARTICLE_H

#include "opengl.h"
#include "linmath.h"

#include "api/random_buffer.h"
#include "api/vector_field.h"
#include <iostream>
#include <utility>

class AppendConsumeBuffer;

class GPUParticle {
public:
  GPUParticle():
    num_alive_particles_(0u),
    pbuffer_(nullptr),
    dp_texture_id_(0u),
    sorted_indices_(0u),
    vao_e_{0u},
    vao_s_{0u, 0u},
    vao_{0u, 0u},
    query_time_(0u),
    simulation_box_size_(kDefaultSimulationBoxSize),
    simulated_(false),
    enable_sorting_(true),
    enable_vectorfield_(true) {enable_sorting_ = true;}

  void init();
  void deinit();

  void update(float const dt, mat4x4 const &view);
  void render(mat4x4 const &view, mat4x4 const &viewProj);

  inline const glm::uvec3& vectorfield_dimensions() const {
    return vectorfield_.dimensions();
  }

  inline float simulation_box_size() const { return simulation_box_size_; }
  inline void simulation_box_size(float size) { simulation_box_size_ = size; }

  inline void enable_sorting(bool status) { enable_sorting_ = status; }
  inline void enable_vectorfield(bool status) { enable_vectorfield_ = status; }

private:
  static unsigned int const kThreadsGroupWidth;

  static unsigned int const kMaxParticleCount = (1u << 16u);
  static unsigned int const kBatchEmitCount = (1u << 11u);
  static float constexpr kDefaultSimulationBoxSize = 256.0f;

  static
  unsigned int GetThreadsGroupCount(unsigned int const nthreads) {
    return (nthreads + kThreadsGroupWidth -1u) / kThreadsGroupWidth;
  }

  static
  unsigned int FloorParticleCount(unsigned int const nparticles){
    return kThreadsGroupWidth * (nparticles / kThreadsGroupWidth);
  }

  void _setup_render();
  void _setup_emission();
  void _setup_simulation();
  void _setup_fill_indices();
  void _setup_calculate_dp();

  void _emission(unsigned int const count);
  void _simulation(float const dt);
  void _postprocess();
  void _sorting(mat4x4 const &view);

  struct {
    float max_age = 1.0f;
  } params_;

  unsigned int num_alive_particles_;  //< number of particle written and rendered on last frame.
  AppendConsumeBuffer *pbuffer_;      //< Append / Consume buffer for particles.

  RandomBuffer randbuffer_;           //< StorageBuffer to hold random values.
  VectorField vectorfield_;           //< Vector field handler.

  struct {
    GLuint emission;
    GLuint update_args;
    GLuint simulation;
    GLuint fill_indices;
    GLuint calculate_dp;
    GLuint sort_step;
    GLuint sort_final;
    GLuint render_point_sprite;
    GLuint render_stretched_sprite;
  } pgm_;               //< Pipeline's shaders.

  struct {
    struct {
      GLint count;
      GLint particleMaxAge;
    } emission;
    struct {
      GLint deltaT;
      GLint vectorFieldSampler;
      GLint bboxSize;
    } simulation;
    struct {
      GLint view;
    } calculate_dp;
    struct {
      GLint width;
      GLint height;
    } fill_indices;
    struct {
      GLint blockWidth;
      GLint maxBlockWidth;
      GLint width;
      GLint height;
      GLint texWidth;
      GLint texHeight;
    } sort_step;
    struct {
      GLint mvp;
    } render_point_sprite;
    struct {
      GLint view;
      GLint mvp;
    } render_stretched_sprite;
  } ulocation_;             //< Programs uniform location.

  struct  {
    struct {
      GLuint randvector;
    } emission;
    struct {
      GLuint position;
      GLuint velocity;
      GLuint age;
      GLuint randvector;
    } simulation;
    struct {
      GLuint position;
      GLuint velocity;
      GLuint age;
    } calculate_dp;
    struct {
      GLuint position;
      GLuint velocity;
      GLuint age;
    } render_stretched_sprite;
  } alocation_;

  ///
  /// @todo instead, use one array in GPUParticle that holds
  /// all storage ids and share them between subclasses.
  ///
  //GLuint gl_indirect_buffer_id_;                //< Indirect Dispatch / Draw buffer.
  GLuint dp_texture_id_;                          //< DotProduct float texture.
  GLuint indices_texture_ids_[2];                     //< indices unsigned int texture (for sorting).

  GLuint texture_width_1;
  GLuint texture_height_1;
  GLuint framebuffer_;

  GLuint sorted_indices_;                             //sorted indices buffer.

  GLuint vao_e_[1];//VAO for emission
  GLuint vao_s_[2];//VAO for simulation
  GLuint vao_c_[2];//VAO for calculating dp
  GLuint vao_[2];                                  //< VAOs rendering.
  GLuint vao_f_;
  GLuint query_time_;                           //< QueryObject for benchmarking.

  GLuint vbo_;
  GLuint vbo_f_;

  GLuint framebuf[2];
  GLuint framebuffer1_;

  float simulation_box_size_;                   //< Boundary used by the simulation, if any.

  bool simulated_;

  bool enable_sorting_;                         //< True if back-to-front sort is enabled.
  bool enable_vectorfield_;                     //< True if the vector field is used.
};

#endif //API_GPU_PARTICLE_H
