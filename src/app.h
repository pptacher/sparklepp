#ifndef DEMO_APP_H_
#define DEMO_APP_H_

#include <chrono>
#include <sstream>
#include "linmath.h"
#include "arcball_camera.h"
#include "scene.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "opengl.h"

struct GLFWwindow;

struct Character {
  GLuint textureID;
  glm::ivec2 size;
  glm::ivec2 bearing;
  GLuint advance;
};

class App {
  public :
  App() : window_(nullptr), deltatime_(0.0f), num_frames_(0){}

  bool init(char const* title);
  void deinit();

  void run();

  private:
    void _frame();
    void _update_camera();
    void _update_time();
    void setup_text();
    void setup_shaders();
    void render_text(std::string, float, float, float, glm::vec3);


    std::chrono::steady_clock::time_point time_;
    std::chrono::steady_clock::time_point start_time_;
    GLFWwindow *window_;

    ArcBallCamera camera_;

    struct {
      mat4x4 view;
      mat4x4 proj;
      mat4x4 viewProj;
    } matrix_;

    struct {
      GLuint text;
    } pgm_;

    struct {
      struct {
        GLint projection;
        GLint color;
      } text;
    } ulocation_;

    Scene scene_;

    std::map<GLchar, Character> characters;

    float deltatime_;
    float num_frames_;
    float fps_;

    GLuint vao_;                    //vao for rendering text.
    GLuint vbo_;
};

#endif
