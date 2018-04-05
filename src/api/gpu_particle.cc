#include "api/gpu_particle.h"
#include "api/append_consume_buffer.h"

#include "shaders/sparkle/interop.h"

#include <iostream>

unsigned int const GPUParticle::kThreadsGroupWidth = PARTICLES_KERNEL_GROUP_WIDTH;
unsigned int const GPUParticle::kBatchEmitCount;

#define _BENCHMARK(block) \
{ \
  unsigned int const kNumIter = 10000u; \
  static unsigned int iter = 0u; \
  static unsigned int result = 0u; \
  glFinish(); \
  { block; } \
  glEndQuery(GL_TIME_ELAPSED); \
  unsigned int dt; glGetQueryObjectuiv(query_time_, GL_QUERY_RESULT, &dt); \
  result += dt / 100; \
  if (++iter == kNumIter) { fprintf(stderr, "%8u | %32s [%u iter, %u p]\n", result / kNumIter, #block, kNumIter, kMaxParticleCount);} \
}

#define BENCHMARK(x) x

#define DBGMARK fprintf(stderr, "%3u\t\t%s\n", __LINE__, __FUNCTION__);

#define _SHOWBUFFER() { \
  GLint params; \
  fprintf(stderr, "%d\n", __LINE__); \
  glGetIntegerv(GL_SHADER_STORAGE_BUFFER_BINDING, &params); \
  for (size_t i = 0; i < COUNT_STORAGE_BINDING; i++) { \
    glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, i, &params); \
    fprintf(stderr, "%u: %u\n", i, params); \
  } \
  fprintf(stderr, "--------------\n"); \
}

#define SHOWBUFFER()

#define SHOWUINT(x)           fprintf(stderr, "%s: %u\n", #x, x);

namespace {
  unsigned int GetClosestPowerOfTwo(unsigned int const n) {
    unsigned int r = 1u;
    for (size_t i = 0; r < n; i++) {
      r <<= 1u;
    }
    return r;
  }

  unsigned int GetNumTrailingBits(unsigned int const n) {
    unsigned int r = 0u;
    for (size_t i = n; i > 1u; i >>= 1u) {
      ++r;
    }
    return r;
  }

} //namespace

void GPUParticle::init() {
  //assert that the number of particles will be a factor of threadGroupWidth.
  unsigned int const num_particles = FloorParticleCount(kMaxParticleCount);
  fprintf(stderr, "[ %u particles, %u per batch ]\n", num_particles, kBatchEmitCount);

  //append consume buffer.
  unsigned int const num_attrib_buffer = 3u; //(sizeof(TParticle) + sizeof(vec4) - 1u) / sizeof(vec4); // = 3 ?
  pbuffer_ = new AppendConsumeBuffer(num_particles, num_attrib_buffer);
  pbuffer_->initialize();

  //random value buffer. generate a random vector for each particle.
  unsigned int const num_randvalues = 3u * num_particles;
  randbuffer_.initialize(num_randvalues);
  randbuffer_.generate_values();

  //vector field generator.
  if (1) {
    vectorfield_.initialize(256u, 256u,256u);
    vectorfield_.generate_values("velocities.dat");
  }

  //compile / link shader programs.
  char *src_buffer = new char [MAX_SHADER_BUFFERSIZE]();
  pgm_.emission = CompileProgram(
        SHADERS_DIR "/sparkle/cs_emission.glsl",
        nullptr,
        src_buffer);
  const char* varyings[3] = { "tfPosition",  "tfVelocity", "tfAge" };
  glTransformFeedbackVaryings(pgm_.emission, 3, varyings, GL_INTERLEAVED_ATTRIBS);
  LinkProgram(pgm_.emission, SHADERS_DIR "/sparkle/cs_emission.glsl");

  pgm_.simulation = CompileProgram(
        SHADERS_DIR "/sparkle/cs_simulation.glsl",
        SHADERS_DIR "/sparkle/gs_simulation.glsl",
        nullptr,
        src_buffer);
  glTransformFeedbackVaryings(pgm_.simulation, 3, varyings, GL_INTERLEAVED_ATTRIBS);
  LinkProgram(pgm_.simulation, SHADERS_DIR "/sparkle/gs_simulation.glsl");

  pgm_.fill_indices = CompileProgram(
        SHADERS_DIR "/sparkle/vs_fill_indices.glsl",
        SHADERS_DIR "/sparkle/fs_fill_indices.glsl",
        src_buffer);
  LinkProgram(pgm_.fill_indices, SHADERS_DIR "/sparkle/fs_fill_indices.glsl");

  pgm_.calculate_dp = CompileProgram(
      SHADERS_DIR "/sparkle/vs_calculate_dp.glsl",
      nullptr,
      src_buffer);
  const char* varyings1[1] = { "tfDp" };
  glTransformFeedbackVaryings(pgm_.calculate_dp, 1, varyings1, GL_INTERLEAVED_ATTRIBS);
  LinkProgram(pgm_.calculate_dp, SHADERS_DIR "/sparkle/vs_calculate_dp.glsl");

  pgm_.sort_step = CompileProgram(
      SHADERS_DIR "/sparkle/vs_sort_step.glsl",
      SHADERS_DIR "/sparkle/fs_sort_step.glsl",
      src_buffer);
  LinkProgram(pgm_.sort_step, SHADERS_DIR "/sparkle/fs_sort_step.glsl");

  /*pgm_.render_point_sprite = CompileProgram(
          SHADERS_DIR "/sparkle/vs_generic.glsl",
          SHADERS_DIR "/sparkle/fs_point_sprite.glsl",
          src_buffer);
  LinkProgram(pgm_.render_point_sprite, SHADERS_DIR "/sparkle/fs_point_sprite.glsl");*/

  pgm_.render_stretched_sprite = CompileProgram(
          SHADERS_DIR "/sparkle/vs_generic.glsl",
          SHADERS_DIR "/sparkle/gs_stretched_sprite.glsl",
          SHADERS_DIR "/sparkle/fs_stretched_sprite.glsl",
          src_buffer);
  LinkProgram(pgm_.render_stretched_sprite, SHADERS_DIR "/sparkle/fs_stretched_sprite.glsl");
  delete[] src_buffer;

  //get attributes location.
  alocation_.emission.randvector = glGetAttribLocation(pgm_.emission, "randvec");
  alocation_.simulation.position = glGetAttribLocation(pgm_.simulation, "position");
  alocation_.simulation.velocity = glGetAttribLocation(pgm_.simulation, "velocity");
  alocation_.simulation.age = glGetAttribLocation(pgm_.simulation, "age");
  alocation_.simulation.randvector = glGetAttribLocation(pgm_.simulation, "randvec");
  alocation_.calculate_dp.position = glGetAttribLocation(pgm_.calculate_dp, "position");
  alocation_.calculate_dp.velocity = glGetAttribLocation(pgm_.calculate_dp, "velocity");
  alocation_.calculate_dp.age = glGetAttribLocation(pgm_.calculate_dp, "age");
  alocation_.render_stretched_sprite.position = glGetAttribLocation(pgm_.render_stretched_sprite, "position");
  alocation_.render_stretched_sprite.velocity = glGetAttribLocation(pgm_.render_stretched_sprite, "velocity");
  alocation_.render_stretched_sprite.age = glGetAttribLocation(pgm_.render_stretched_sprite, "age_info");

  //get uniform location.
  ulocation_.emission.count = GetUniformLocation(pgm_.emission, "uEmitCount");
  ulocation_.emission.particleMaxAge = GetUniformLocation(pgm_.emission, "uParticleMaxAge");
  ulocation_.simulation.deltaT = GetUniformLocation(pgm_.simulation, "uDeltaT");
  ulocation_.simulation.vectorFieldSampler = GetUniformLocation(pgm_.simulation, "uVectorFieldSampler");
  ulocation_.simulation.bboxSize = GetUniformLocation(pgm_.simulation, "uBBoxSize");
  ulocation_.calculate_dp.view = GetUniformLocation(pgm_.calculate_dp, "uViewMatrix");
  ulocation_.fill_indices.width = GetUniformLocation(pgm_.fill_indices, "width");
  ulocation_.fill_indices.height = GetUniformLocation(pgm_.fill_indices, "height");
  ulocation_.sort_step.blockWidth = GetUniformLocation(pgm_.sort_step, "uBlockWidth");
  ulocation_.sort_step.maxBlockWidth = GetUniformLocation(pgm_.sort_step, "uMaxBlockWidth");
  ulocation_.sort_step.width = GetUniformLocation(pgm_.sort_step, "width");
  ulocation_.sort_step.height = GetUniformLocation(pgm_.sort_step, "height");
  ulocation_.sort_step.texWidth = GetUniformLocation(pgm_.sort_step, "texWidth");
  ulocation_.sort_step.texHeight = GetUniformLocation(pgm_.sort_step, "texHeight");
  //ulocation_.render_point_sprite.mvp = GetUniformLocation(pgm_.render_point_sprite, "uMVP");
  ulocation_.render_stretched_sprite.view = GetUniformLocation(pgm_.render_stretched_sprite, "uView");
  ulocation_.render_stretched_sprite.mvp = GetUniformLocation(pgm_.render_stretched_sprite, "uMVP");

  //one time uniform setting.
  glProgramUniform1i(pgm_.simulation,
                      GetUniformLocation(pgm_.simulation, "uPerlinNoisePermutationSeed"),
                    rand());

  GLuint ln_size = (GLuint)(std::log2(kMaxParticleCount)/ 2);
  texture_width_1 = 1 << (GLuint) (ln_size);
  texture_height_1 = 1 << (GLuint)(std::log2(kMaxParticleCount) - ln_size);

  // texture for indices sorting.
  glGenTextures(2, indices_texture_ids_);

  glBindTexture(GL_TEXTURE_2D, indices_texture_ids_[0]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, texture_width_1, texture_height_1, 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, nullptr);
  glBindTexture(GL_TEXTURE_2D, indices_texture_ids_[1]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, texture_width_1, texture_height_1, 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, nullptr);

  //framebuffer.
  glGenFramebuffers(1, &framebuffer_);

  //dot products float texture.
  glGenTextures(1, &dp_texture_id_);

  //set up VAOs.
  _setup_emission();
  _setup_simulation();
  _setup_render();

  //query used for the benchmarking.
  glGenQueries(1, &query_time_);
  glBeginQuery(GL_TIME_ELAPSED, query_time_);

  CHECKGLERROR();
}

void GPUParticle::deinit() {
  pbuffer_->deinitialize();
  delete pbuffer_;

  randbuffer_.deinitialize();

  if (enable_vectorfield_) {
    vectorfield_.deinitialize();
  }

  glDeleteProgram(pgm_.emission);
  glDeleteProgram(pgm_.update_args);
  glDeleteProgram(pgm_.simulation);
  glDeleteProgram(pgm_.fill_indices);
  glDeleteProgram(pgm_.calculate_dp);
  glDeleteProgram(pgm_.sort_step);
  glDeleteProgram(pgm_.sort_final);
  glDeleteProgram(pgm_.render_point_sprite);

  glDeleteVertexArrays(1u, vao_e_);
  glDeleteVertexArrays(2u, vao_s_);
  glDeleteVertexArrays(2u, vao_);
  glDeleteQueries(1u,&query_time_);
}

void GPUParticle::update(const float dt, mat4x4 const &view) {
  //max number of particles able to be spawned.
  unsigned int const num_dead_particles = pbuffer_->element_count() - num_alive_particles_;
  //number of particles to be emitted.
  unsigned int const emit_count = std::min(kBatchEmitCount, num_dead_particles);

  //update random buffer with new values.too heavy.
  //randbuffer_.generate_values();

  //emission stage: write in buffer A.
  _emission(emit_count);

  //simulation stage: read buffer A, write buffer B.
  _simulation(dt);
//std::cout << "simulated:" << enable_sorting_ << '\n';
  //sort particles for alpha-blending.
  if (1 and simulated_) {
    _sorting(view);
  }
  //post process stage.
  _postprocess();

  CHECKGLERROR();
}

void GPUParticle::render(mat4x4 const &view, mat4x4 const &viewProj) {
  #if 1
  glUseProgram(pgm_.render_stretched_sprite);
  {
    glUniformMatrix4fv(ulocation_.render_stretched_sprite.view, 1, GL_FALSE, (GLfloat *const) view);
    glUniformMatrix4fv(ulocation_.render_stretched_sprite.mvp, 1, GL_FALSE, (GLfloat *const)viewProj);
  #else
    glUseProgram(pgm_.render_point_sprite);
    {
      glUniformMatrix4fv(ulocation_.render_point_sprite.mvp,  1, GL_FALSE, (GLfloat *const)viewProj);
      #endif

/*      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sorted_indices_);
      GLushort *data5 = (GLushort*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_ONLY);
      std::cout <<'\n' << "indices before rendering: " << '\n';
      for (size_t i = 0; i < GetClosestPowerOfTwo(num_alive_particles_); i++) {
        std::cout << data5[i] << " ";
      }
      std::cout <<  std::endl;
      glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

      glBindBuffer(GL_ARRAY_BUFFER, pbuffer_->first_array_buffer_id());
      GLfloat *data4 = (GLfloat*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
      std::cout <<'\n' << "vboA before rendering: " << '\n';
      for (size_t i = 0; i < GetClosestPowerOfTwo(num_alive_particles_); i++) {

        std::cout << "position :" ;
        for (size_t j = 0; j < 3; j++) {
          std::cout << data4[8u*i+j] << ' ';
        }
        std::cout  << '\t';
        std::cout << "speed :" ;
        for (size_t j = 0; j < 3; j++) {
          std::cout << data4[8u*i+3+j] << ' ';
        }
        std::cout  << '\t';
        std::cout << "age :" ;
        for (size_t j = 0; j < 2; j++) {
          std::cout << data4[8u*i+6+j] << ' ';
        }
        std::cout  << '\n';
      }
      glUnmapBuffer(GL_ARRAY_BUFFER);*/

    glBindVertexArray(vao_[0]);
      glDrawElements(GL_POINTS, num_alive_particles_, GL_UNSIGNED_SHORT, 0);
      //glDrawArrays(GL_POINTS, 0, num_alive_particles_);
    glBindVertexArray(0u);
  }
  glUseProgram(0u);


  CHECKGLERROR();
}

void GPUParticle::_setup_emission() {
  glGenVertexArrays(1u,&vao_e_[0]);
  glBindVertexArray(vao_e_[0]);

  randbuffer_.bind();{
    glVertexAttribPointer(alocation_.emission.randvector, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(alocation_.emission.randvector);
  }
randbuffer_.unbind();
  glBindVertexArray(0);

  CHECKGLERROR();
}

void GPUParticle::_setup_simulation() {
  glGenVertexArrays(2u, vao_s_);

  glBindVertexArray(vao_s_[0]);
  GLuint vbo1 = pbuffer_->first_array_buffer_id();

  glBindBuffer(GL_ARRAY_BUFFER, vbo1); {
    glVertexAttribPointer(alocation_.simulation.position, 3, GL_FLOAT, GL_FALSE,8 * sizeof(GLfloat) , nullptr);
    glEnableVertexAttribArray(alocation_.simulation.position);

    glVertexAttribPointer(alocation_.simulation.velocity, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat),(void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(alocation_.simulation.velocity);

    glVertexAttribPointer(alocation_.simulation.age, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(alocation_.simulation.age);
  }

  glBindVertexArray(vao_s_[1]);
  GLuint vbo2 = pbuffer_->second_array_buffer_id();

  glBindBuffer(GL_ARRAY_BUFFER, vbo2); {
    glVertexAttribPointer(alocation_.simulation.position, 3, GL_FLOAT, GL_FALSE,8 * sizeof(GLfloat) , nullptr);
    glEnableVertexAttribArray(alocation_.simulation.position);

    glVertexAttribPointer(alocation_.simulation.velocity, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat),(void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(alocation_.simulation.velocity);

    glVertexAttribPointer(alocation_.simulation.age, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(alocation_.simulation.age);
  }

  // #if ENABLE_SCATTERING
  /*randbuffer_.bind();{
    std::cout << "randvector: " << alocation_.simulation.randvector << '\n';
    glVertexAttribPointer(alocation_.simulation.randvector, 3, GL_FLOAT, GL_FALSE,0 , nullptr);
    glEnableVertexAttribArray(alocation_.simulation.randvector);
  }*/
  //randbuffer_.unbind();
  glBindVertexArray(0u);

  CHECKGLERROR();
}

void GPUParticle::_setup_render() {

  glGenBuffers(1u, &sorted_indices_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sorted_indices_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, FloorParticleCount(kMaxParticleCount) * sizeof(GLushort), nullptr, GL_DYNAMIC_DRAW);

  glGenVertexArrays(2u, vao_);

  glBindVertexArray(vao_[0]);

  GLuint const vboA = pbuffer_->first_array_buffer_id();

  glBindBuffer(GL_ARRAY_BUFFER, vboA); {
    glVertexAttribPointer(alocation_.render_stretched_sprite.position, 3, GL_FLOAT, GL_FALSE,8 * sizeof(GLfloat) , nullptr);
    glEnableVertexAttribArray(alocation_.render_stretched_sprite.position);

    glVertexAttribPointer(alocation_.render_stretched_sprite.velocity, 3, GL_FLOAT, GL_FALSE,8 * sizeof(GLfloat),(void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(alocation_.render_stretched_sprite.velocity);

    glVertexAttribPointer(alocation_.render_stretched_sprite.age, 2, GL_FLOAT, GL_FALSE,8 * sizeof(GLfloat),(void*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(alocation_.render_stretched_sprite.age);
  }
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sorted_indices_);

  glBindVertexArray(vao_[1]);

  GLuint const vboB = pbuffer_->second_array_buffer_id();

  glBindBuffer(GL_ARRAY_BUFFER, vboB); {
    glVertexAttribPointer(alocation_.render_stretched_sprite.position, 3, GL_FLOAT, GL_FALSE,8 * sizeof(GLfloat) , nullptr);
    glEnableVertexAttribArray(alocation_.render_stretched_sprite.position);

    glVertexAttribPointer(alocation_.render_stretched_sprite.velocity, 3, GL_FLOAT, GL_FALSE,8 * sizeof(GLfloat),(void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(alocation_.render_stretched_sprite.velocity);

    glVertexAttribPointer(alocation_.render_stretched_sprite.age, 2, GL_FLOAT, GL_FALSE,8 * sizeof(GLfloat),(void*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(alocation_.render_stretched_sprite.age);
  }
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sorted_indices_);
  //glBindBuffer(GL_ARRAY_BUFFER, 0u);

  glBindVertexArray(0u);

  CHECKGLERROR();
}

void GPUParticle::_emission(const unsigned int count){
  //emit only if a minimum count is reached.
  if (!count || (count < kBatchEmitCount)) {
    return;
  }
  GLuint vboA = pbuffer_->first_array_buffer_id();

/*  glBindBuffer(GL_ARRAY_BUFFER, vboA);
  GLfloat *data4 = (GLfloat*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
  std::cout <<'\n' << "vboA before emission: " << '\n';
  for (size_t i = 0; i < num_alive_particles_; i++) {

    std::cout << "position :" ;
    for (size_t j = 0; j < 3; j++) {
      std::cout << data4[8u*i+j] << ' ';
    }
    std::cout  << '\t';
    std::cout << "speed :" ;
    for (size_t j = 0; j < 3; j++) {
      std::cout << data4[8u*i+3+j] << ' ';
    }
    std::cout  << '\t';
    std::cout << "age :" ;
    for (size_t j = 0; j < 2; j++) {
      std::cout << data4[8u*i+6+j] << ' ';
    }
    std::cout  << '\n';
  }
  glUnmapBuffer(GL_ARRAY_BUFFER);
  glBindBuffer(GL_ARRAY_BUFFER, 0);*/

  glBindVertexArray(vao_e_[0]);

  glUseProgram(pgm_.emission);
  {

    glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER,
                      0,
                      vboA,
                      num_alive_particles_ * 8 * sizeof(GLfloat),
                      count * 8 * sizeof(GLfloat));
    glUniform1ui(ulocation_.emission.count, count);
    glUniform1f(ulocation_.emission.particleMaxAge, params_.max_age);

    glEnable(GL_RASTERIZER_DISCARD);
    glBeginTransformFeedback(GL_POINTS);
      glDrawArrays(GL_POINTS, num_alive_particles_, count);
    glEndTransformFeedback();

    glDisable(GL_RASTERIZER_DISCARD);
  }
  glUseProgram(0u);
  glBindVertexArray(0);

  //number of particles expected to be simulated.
  num_alive_particles_ += count;

/*  glBindBuffer(GL_ARRAY_BUFFER, vboA);
  GLfloat *data5 = (GLfloat*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
  std::cout <<'\n' << "vboA after emission: " << '\n';
  for (size_t i = 0; i < num_alive_particles_; i++) {

    std::cout << "position :" ;
    for (size_t j = 0; j < 3; j++) {
      std::cout << data5[8u*i+j] << ' ';
    }
    std::cout  << '\t';
    std::cout << "speed :" ;
    for (size_t j = 0; j < 3; j++) {
      std::cout << data5[8u*i+3+j] << ' ';
    }
    std::cout  << '\t';
    std::cout << "age :" ;
    for (size_t j = 0; j < 2; j++) {
      std::cout << data5[8u*i+6+j] << ' ';
    }
    std::cout  << '\n';
  }
  glUnmapBuffer(GL_ARRAY_BUFFER);*/

  CHECKGLERROR();
}

void GPUParticle::_simulation(float const dt) {
  if (num_alive_particles_ == 0u) {
    simulated_ = false;
    return;
  }

  //simulation kernel.
  if (enable_vectorfield_) {
    glBindTexture(GL_TEXTURE_3D, vectorfield_.texture_id());
  }

  GLuint vboB = pbuffer_->second_array_buffer_id();

  /*glBindBuffer(GL_ARRAY_BUFFER, pbuffer_->first_array_buffer_id());
  GLfloat *data4 = (GLfloat*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
  std::cout <<'\n' << "vboA before simulating: " << '\n';
  for (size_t i = 0; i < num_alive_particles_; i++) {

    std::cout << "position :" ;
    for (size_t j = 0; j < 3; j++) {
      std::cout << data4[8u*i+j] << ' ';
    }
    std::cout  << '\t';
    std::cout << "speed :" ;
    for (size_t j = 0; j < 3; j++) {
      std::cout << data4[8u*i+3+j] << ' ';
    }
    std::cout  << '\t';
    std::cout << "age :" ;
    for (size_t j = 0; j < 2; j++) {
      std::cout << data4[8u*i+6+j] << ' ';
    }
    std::cout  << '\n';
  }
  std::cout <<  std::endl;
  glUnmapBuffer(GL_ARRAY_BUFFER);
  glBindBuffer(GL_ARRAY_BUFFER, 0);*/

  GLuint particles_passed = 0u;
  GLuint particles_query = 0u;

  glBindVertexArray(vao_s_[0]);
  glUseProgram(pgm_.simulation);
  {
    glEnable(GL_RASTERIZER_DISCARD);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vboB);
    glUniform1f(ulocation_.simulation.deltaT, dt);
    glUniform1i(ulocation_.simulation.vectorFieldSampler, 0);
    glUniform1f(ulocation_.simulation.bboxSize, simulation_box_size_);
    glActiveTexture( GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, vectorfield_.texture_id());
    glGenQueries(1, &particles_query);
    glBeginQuery(GL_PRIMITIVES_GENERATED, particles_query);

    glBeginTransformFeedback(GL_POINTS);
      glDrawArrays(GL_POINTS, 0, num_alive_particles_);
    glEndTransformFeedback();
    glDisable(GL_RASTERIZER_DISCARD);

    glEndQuery(GL_PRIMITIVES_GENERATED);
    glGetQueryObjectuiv(particles_query, GL_QUERY_RESULT, &particles_passed);
    glDeleteQueries(1, &particles_query);
    //glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
  }
  glUseProgram(0u);
  glBindVertexArray(0);

  glBindTexture(GL_TEXTURE_3D, 0u);
  num_alive_particles_ = particles_passed;
  /*glBindBuffer(GL_ARRAY_BUFFER, pbuffer_->second_array_buffer_id());
  GLfloat *data5 = (GLfloat*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
  std::cout <<'\n' << "vboB after simulating: " << '\n';
  for (size_t i = 0; i < num_alive_particles_; i++) {

    std::cout << "position :" ;
    for (size_t j = 0; j < 3; j++) {
      std::cout << data5[8u*i+j] << ' ';
    }
    std::cout  << '\t';
    std::cout << "speed :" ;
    for (size_t j = 0; j < 3; j++) {
      std::cout << data5[8u*i+3+j] << ' ';
    }
    std::cout  << '\t';
    std::cout << "age :" ;
    for (size_t j = 0; j < 2; j++) {
      std::cout << data5[8u*i+6+j] << ' ';
    }
    std::cout  << '\n';
  }
  std::cout <<  std::endl;
  glUnmapBuffer(GL_ARRAY_BUFFER);
  glBindBuffer(GL_ARRAY_BUFFER, 0);*/

  simulated_ = true;

  CHECKGLERROR();
}

void GPUParticle::_sorting(mat4x4 const &view) {

  unsigned int const max_elem_count = GetClosestPowerOfTwo(num_alive_particles_);
  GLuint ln_size = (GLuint)(std::log2(max_elem_count)/ 2);
  GLuint texture_width_ = 1 << (GLuint) (ln_size);
  GLuint texture_height_ = 1 << (GLuint)(std::log2(max_elem_count) - ln_size);

  glActiveTexture( GL_TEXTURE0 );
  glBindTexture(GL_TEXTURE_2D, indices_texture_ids_[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


  //write to texture.
  glActiveTexture( GL_TEXTURE0 + 1);
  glBindTexture(GL_TEXTURE_2D, indices_texture_ids_[1]);

  //framebuffer.
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
  glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, indices_texture_ids_[0], 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  GLuint clear_value1[] = {0, 0, 0, 0};
  glClearBufferuiv(GL_COLOR, 0, clear_value1);
  glViewport(0,0,texture_width_, texture_height_);
/*glBindTexture(GL_TEXTURE_2D, indices_texture_ids_[0]);
  GLushort *data1 = (GLushort*)malloc(kMaxParticleCount * sizeof(GL_UNSIGNED_SHORT));
  glReadPixels(0.0f, 0.0f, texture_width_1, texture_height_1, GL_RED_INTEGER, GL_UNSIGNED_SHORT, data1);
  std::cout <<'\n' << "filled indices: " << '\n';
  for (size_t i = 0; i < kMaxParticleCount; i += 1) {
    if (i % (texture_width_1) == 0)
      std::cout  << '\n';
    std::cout << data1[i] << " ";
  }
  std::cout <<  std::endl;
  free(data1);*/

  glActiveTexture( GL_TEXTURE0 + 2);
  glBindTexture(GL_TEXTURE_2D, dp_texture_id_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, texture_width_1, texture_height_1, 0, GL_RED, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glBindTexture(GL_TEXTURE_2D, 0);

  GLfloat vertices[] = {
      -1.0f, -1.0f, 0.0f, 0.0f,
       1.0f, -1.0f, 1.0f  , 0.0f,
       1.0f, 1.0f, 1.0f, 1.0f,
       -1.0f, 1.0f, 0.0f, 1.0f
  };

  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  GLuint vbo;
  glGenBuffers(1, &vbo);
  /* 1) Intialize indices and dotproducts buffers. */
  // Fill first part of the indices buffer with continuous indices.
  glUseProgram(pgm_.fill_indices);
  {
    glUniform1ui(ulocation_.fill_indices.width, texture_width_);
    glUniform1ui(ulocation_.fill_indices.height, texture_height_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    GLint posAttrib = glGetAttribLocation(pgm_.fill_indices, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    GLint texAttrib = glGetAttribLocation(pgm_.fill_indices, "texcoord");
    glEnableVertexAttribArray(texAttrib);
    glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);

    glDrawArrays(GL_TRIANGLE_FAN,0, 4);

CHECKGLERROR();
  }
  glUseProgram(0u);
  glBindVertexArray(0u);
//glBindFramebuffer(GL_FRAMEBUFFER, 0);
/*
glBindTexture(GL_TEXTURE_2D, indices_texture_ids_[0]);
GLushort *data = (GLushort*)malloc(kMaxParticleCount * sizeof(GL_UNSIGNED_SHORT));
glReadPixels(0.0f, 0.0f, texture_width_1, texture_height_1, GL_RED_INTEGER, GL_UNSIGNED_SHORT, data);
std::cout <<'\n' << "filled indices: " << '\n';
for (size_t i = 0; i < kMaxParticleCount; i += 1) {
  if (i % (texture_width_1) == 0)
    std::cout  << '\n';
  std::cout << data[i] << " ";
}
std::cout <<  std::endl;
free(data);*/

  //clear the dot product buffer.
  GLfloat const clear_value[] = {-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX} ;

  GLuint framebuffer1_;
  glGenFramebuffers(1, &framebuffer1_);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer1_);
  glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dp_texture_id_, 0);
  glClearBufferfv(GL_COLOR, 0, clear_value);

  //compute dot products of particles toward the camera.
  glUseProgram(pgm_.calculate_dp);
  {
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * max_elem_count, nullptr, GL_STATIC_DRAW);

    GLuint vboB = pbuffer_->second_array_buffer_id();

    glBindBuffer(GL_ARRAY_BUFFER, vboB); {

      glVertexAttribPointer(alocation_.calculate_dp.position, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), nullptr);
      glEnableVertexAttribArray(alocation_.calculate_dp.position);

      /// @note No kernel boundaries check performed.
      glUniformMatrix4fv(ulocation_.calculate_dp.view, 1, GL_FALSE, (GLfloat *const)view);
      glEnable(GL_RASTERIZER_DISCARD);
      glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vbo);
      glBeginTransformFeedback(GL_POINTS);
        glDrawArrays(GL_POINTS, 0, max_elem_count);
      glEndTransformFeedback();
      glDisable(GL_RASTERIZER_DISCARD);

      glBindTexture(GL_TEXTURE_2D, dp_texture_id_);
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, vbo);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture_width_, texture_height_, GL_RED, GL_FLOAT, 0);
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
/*
      glBindTexture(GL_TEXTURE_2D, dp_texture_id_);

      GLfloat *data2 = (GLfloat*)malloc(kMaxParticleCount * sizeof(GL_FLOAT));
      glReadPixels(0.0f, 0.0f, texture_width_1, texture_height_1, GL_RED, GL_FLOAT, data2);
      std::cout <<'\n' << "dot products: " << '\n';
      for (size_t i = 0; i < kMaxParticleCount; i += 1) {
        if (i % (texture_width_1) == 0)
          std::cout  << '\n';
        std::cout << data2[i] << " ";
      }
      std::cout <<  std::endl;
      free(data2);*/

      glBindFramebuffer(GL_FRAMEBUFFER, 0u);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0u);
  }
  glUseProgram(0u);



CHECKGLERROR();

  unsigned int const nsteps = GetNumTrailingBits(max_elem_count);
  uint dp_texture_location = glGetUniformLocation(pgm_.sort_step, "dp");
  uint indices_texture_location = glGetUniformLocation(pgm_.sort_step, "indices");

  glUseProgram(pgm_.sort_step);
  glUniform1i(dp_texture_location, 0);
  glUniform1i(indices_texture_location, 1);
  glUniform1ui(ulocation_.sort_step.width, texture_width_);
  glUniform1ui(ulocation_.sort_step.height, texture_height_);

  GLuint framebuf[2];
  glGenFramebuffers(2u, framebuf);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuf[0]);
  glBindTexture(GL_TEXTURE_2D, indices_texture_ids_[1]);
  glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, indices_texture_ids_[1], 0);
  glClearBufferuiv(GL_COLOR, 0, clear_value1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuf[1]);
  glBindTexture(GL_TEXTURE_2D, indices_texture_ids_[0]);
  glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, indices_texture_ids_[0], 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuf[0]);

  glActiveTexture( GL_TEXTURE0 );
  glBindTexture(GL_TEXTURE_2D, dp_texture_id_);
  glActiveTexture( GL_TEXTURE0 + 1);
  glBindTexture(GL_TEXTURE_2D, indices_texture_ids_[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glBindVertexArray(vao);

  glUniform1ui(ulocation_.sort_step.texWidth, texture_width_1);
  glUniform1ui(ulocation_.sort_step.texHeight, texture_height_1);

  unsigned int binding = 0u;
  for (size_t step = 0; step < nsteps; step++) {
    for (size_t stage = 0; stage < step + 1u; stage++) {

      //compute kernel parameters.
      GLuint const block_width = 2u << (step - stage);
      GLuint const max_block_width = 2u << step;
      glUniform1ui(ulocation_.sort_step.blockWidth, block_width);
      glUniform1ui(ulocation_.sort_step.maxBlockWidth, max_block_width);

      glBindFramebuffer(GL_FRAMEBUFFER, framebuf[binding]);
      glActiveTexture( GL_TEXTURE0  );
      glBindTexture(GL_TEXTURE_2D, dp_texture_id_);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glActiveTexture( GL_TEXTURE0 + 1u );
      glBindTexture(GL_TEXTURE_2D, indices_texture_ids_[binding]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      binding ^= 1u;

      glDrawArrays(GL_TRIANGLE_FAN,0, 4);
/*
      glBindTexture(GL_TEXTURE_2D, indices_texture_ids_[binding]);
      GLushort *data5 = (GLushort*)malloc(kMaxParticleCount * sizeof(GL_UNSIGNED_SHORT));
      glReadPixels(0.0f, 0.0f, texture_width_1, texture_height_1, GL_RED_INTEGER, GL_UNSIGNED_SHORT, data5);
      std::cout <<'\n' << "stage: " << stage << '\n';
      for (size_t i = 0; i < kMaxParticleCount; i += 1) {
        if (i % (texture_width_1) == 0)
          std::cout  << '\n';
        std::cout << data5[i] << " ";
      }
      std::cout <<  std::endl;
      free(data5);*/

CHECKGLERROR();
    }
  }
  glUseProgram(0u);
  glBindVertexArray(0u);

  glBindTexture(GL_TEXTURE_2D, indices_texture_ids_[binding]);

  glBindBuffer(GL_PIXEL_PACK_BUFFER, sorted_indices_);
    glReadPixels(0, 0, texture_width_, texture_height_, GL_RED_INTEGER, GL_UNSIGNED_SHORT, 0);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0u);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
/*
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sorted_indices_);
  GLushort *data7 = (GLushort*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_ONLY);
  std::cout <<'\n' << "indices after sort in element array buffer: " << '\n';
  for (size_t i = 0; i < max_elem_count; i++) {
    std::cout << data7[i] << " ";
  }
  std::cout <<  std::endl;
  glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);*/

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  glBindTexture(GL_TEXTURE_2D, 0);
  CHECKGLERROR();

}

void GPUParticle::_postprocess() {
  if (1 || simulated_) {

    //copy non sorted alive particles back to the first buffer.
    if (1) {
      pbuffer_->swap_storage();
      std::swap(vao_s_[0], vao_s_[1]);
      std::swap(vao_[0], vao_[1]);
    }
  }

  /* Copy the number of alive particles to the indirect buffer for drawing. */
  /// @note We use the update_args kernel instead, loosing 1 frame of accuracy.
  //glCopyNamedBufferSubData(
    //pbuffer_->first_atomic_buffer_id(), gl_indirect_buffer_id_, 0u, offsetof(TIndirectValues, draw_count), sizeof(GLuint));

    CHECKGLERROR();
}
