#include "app.h"

#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <iomanip>
#include "GLFW/glfw3.h"

#include "events.h"

bool App::init(char const* title) {

  std::setbuf(stderr, nullptr);
  std::srand(std::time(0));

  if (!glfwInit()) {
    fprintf(stderr, "Error : failed to initialize GLFW.\n");
    return false;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, 0);

    // Compute window resolution from the main monitor's.
    float const scale = 2.0f / 3.0f;
    GLFWvidmode const* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int const w = static_cast<int>(scale * mode->width);
    int const h = static_cast<int>(scale * mode->height);

    //create the window and OpenGL context.
    window_ = glfwCreateWindow(w, h, title, nullptr, nullptr);
    if (!window_) {
      fprintf(stderr, "Error : failed to create the window.\n");
      glfwTerminate();
      return false;
    }

    // Make the window's context current.
    glfwMakeContextCurrent(window_);

    //Init the event manager.
    InitEvents(window_);

    //Initialize OpenGL extensions.
    InitGL();

    //camera setup.
    camera_.dolly(125.0f);

    //setup the projection matrix.
    float const aspectRatio = w / static_cast<float>(h);
    mat4x4_perspective(matrix_.proj, degreesToRadians(60.0f), aspectRatio, 0.01f, 2000.0f);

    //Initialize the scene.
    scene_.init();

    setup_shaders();
    setup_text();

    //start the demo.
    time_ = std::chrono::steady_clock::now();
    start_time_ = std::chrono::steady_clock::now();


    return true;
}

void App::deinit() {
  glfwTerminate();

  window_ = nullptr;
  scene_.deinit();
}

void App::run() {
  //Main loop.

  while (!glfwWindowShouldClose(window_)) {
    //Manage events.
    HandleEvents();

    //Update and render one frame.
    _frame();

    //Swap front & back buffers.
    glfwSwapBuffers(window_);
  }
}

void App::_frame() {

  //Update chrono and calculate deltatim.
  _update_time();

  //camera event handling and matrices update.
  _update_camera();

  //update scene (eg. particles simulation).
  scene_.update(matrix_.view, deltatime_);

  // Compute window resolution from the main monitor's.
  float const scale = 2.0f / 3.0f;
  GLFWvidmode const* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
  int const w = static_cast<int>(scale * mode->width);
  int const h = static_cast<int>(scale * mode->height);
  glViewport(0, 0, w, h);

  //render scene.
  scene_.render(matrix_.view, matrix_.viewProj);

  //render framerate.
  std::stringstream fps_stream;
  fps_stream << std::fixed << std::setprecision(1) << fps_;
  render_text(fps_stream.str() + "\tfps", 25.0f, 25.0f, 1.0f, glm::vec3(0.85f, 0.85f, 0.85f));
}

void App::_update_camera() {
  TEventData const event = GetEventData();

  //update camera.
  camera_.event(
    event.bMouseMove, event.bTranslatePressed, event.bRotatePressed,
    event.mouseX, event.mouseY, event.wheelDelta);

  //compute the view matrix.
  mat4x4_identity(matrix_.view);

  vec3 eye    = {camera_.dolly(), 0.65f * camera_.dolly(), camera_.dolly()};
  vec3 center = {0.0f, 0.0f, 0.0f};
  vec3 up     = {0.0f, 1.0f, 0.0f};
  mat4x4_look_at(matrix_.view, eye, center, up);
  mat4x4_translate_in_place(matrix_.view, camera_.translate_x(), camera_.translate_y(), 0.0f);
  mat4x4 rX;
  mat4x4_rotate_X(rX, matrix_.view, camera_.yaw());
  mat4x4_rotate_Y(matrix_.view, rX, camera_.pitch());

  //update the viewproj matrix.
  mat4x4_mul(matrix_.viewProj, matrix_.proj, matrix_.view);
}

void App::_update_time() {
  std::chrono::steady_clock::time_point tick = std::chrono::steady_clock::now();
  fps_ =  (++num_frames_) / (float) std::chrono::duration_cast<std::chrono::duration<double>>(tick-start_time_).count();
  std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(tick - time_);
  time_ = tick;

  deltatime_ = (GetEventData().bSpacePressed) ? 0.0f : time_span.count();
  deltatime_ *= 1.0f;
}


void App::setup_text() {
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  //segmenataiton fault when using this.

  FT_Library ft;
  if (FT_Init_FreeType(&ft)) {
    std::cout << "Error: freetype: Could not init FreeType Library." << '\n';
  }

  FT_Face face;
  if (FT_New_Face(ft, "/Library/fonts/AmericanTypewriter.ttc", 0, &face)) {
    std::cout << "Error: freetype: Failed to load font." << '\n';
  }

  FT_Set_Pixel_Sizes(face, 0, 12);

  for (GLubyte c = 0; c  < 128; c++) {
    //load character glyph
    if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
      std::cout << "Error: freetype: Failed to load Glyph." << '\n';
      continue;
    }
    //generate texture.
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D,
                0,
                GL_R16F,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    Character character = {
      texture,
      glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
      glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
      (GLuint)face->glyph->advance.x
    };

    characters.insert(std::pair<GLchar, Character>(c, character));

  }

  FT_Done_Face(face);
  FT_Done_FreeType(ft);

  glGenVertexArrays(1, &vao_);
  glGenBuffers(1, &vbo_);
  glBindVertexArray(vao_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 4 * 4, nullptr, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  CHECKGLERROR();
}

void App::setup_shaders() {
  //setup programs.
  char *src_buffer = new char[MAX_SHADER_BUFFERSIZE]();
  pgm_.text = CompileProgram(SHADERS_DIR "/text/vs_text.glsl", SHADERS_DIR "/text/fs_text.glsl", src_buffer);
  LinkProgram(pgm_.text, SHADERS_DIR "/text/fs_text.glsl");
  delete[] src_buffer;

  ulocation_.text.projection = GetUniformLocation(pgm_.text, "projection");
  ulocation_.text.color = GetUniformLocation(pgm_.text, "textColor");

  CHECKGLERROR();
}

void App::render_text(std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color) {

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  float const scale1 = 2.0f / 3.0f;
  GLFWvidmode const* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
  int const w = static_cast<int>(scale1 * mode->width);
  int const h = static_cast<int>(scale1 * mode->height);

  glm::mat4 projection = glm::ortho(0.0f, (float)w, 0.0f, (float)h);
  glUseProgram(pgm_.text);
  glUniform3f(ulocation_.text.color, color.x, color.y, color.z);
  glUniformMatrix4fv(ulocation_.text.projection, 1, GL_FALSE, glm::value_ptr(projection));
  glActiveTexture(GL_TEXTURE0);
  glBindVertexArray(vao_);

  std::string::const_iterator c;
  for (c = text.begin(); c != text.end(); c++) {
    Character ch = characters[*c];

    GLfloat xpos = x + ch.bearing.x * scale;
    GLfloat ypos = y - (ch.size.y - ch.bearing.y) * scale;

    GLfloat w = ch.size.x * scale;
    GLfloat h = ch.size.y * scale;
/*
    GLfloat vertices[6][4] = {
      {xpos, ypos + h, 0.0, 1.0},
      {xpos, ypos, 0.0, 0.0},
      {xpos + w, ypos, 1.0, 0.0},
      {xpos, ypos + h, 0.0, 1.0},
      {xpos + w, ypos, 1.0, 0.0},
      {xpos + w, ypos + h, 1.0, 1.0}
    };*/

    GLfloat vertices[4][4] = {
      {xpos, ypos, 0.0f, 1.0f},
      {xpos + w, ypos, 1.0f, 1.0f},
      {xpos + w, ypos + h, 1.0f, 0.0f},
      {xpos, ypos + h, 0.0f, 0.0f}
    };
    //render glyph texture over quad.
    glBindTexture(GL_TEXTURE_2D, ch.textureID);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    x += (ch.advance >> 6) * scale;
  }
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);

  CHECKGLERROR();
}
