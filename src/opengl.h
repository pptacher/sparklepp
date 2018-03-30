#ifndef DEMO_OPENGL_H_
#define DEMO_OPENGL_H_

/* Use GLEW if enabled, otherwise load extensions manually. */
#ifdef USE_GLEW
#include "GL/glew.h"
#endif

#include "GLFW/glfw3.h"


#ifndef GLEW_VERSION_4_3
#include "ext/_extensions.h"
#endif

// OPENGL debug macros
#ifdef NDEBUG
#define CHECKGLERROR()
#else
#define CHECKGLERROR()    CheckGLError(__FILE__, __LINE__, "", true)
#endif


//automatically specified by CMAKE. safeguard in case.
#ifndef SHADERS_DIR
#define SHADERS_DIR   "../shaders"
#endif

//maximum size per shader file (with include). 64Ko.
#define MAX_SHADER_BUFFERSIZE (64u * 1024u)

void InitGL();
GLuint CompileProgram(char const* vsfile, const char *gsfile, char const *fsfile, char *src_buffer);
GLuint CompileProgram(char const *vsfile, char const *fsfile, char *src_buffer);
void LinkProgram(GLuint pgm, char const *fsfile);
void CheckShaderStatus(GLuint shader, char const *name);
bool CheckProgramStatus(GLuint program, char const *name);
void CheckGLError(char const *file, int const line, char const *errMsg, bool bExitOnFail);
bool IsBufferBound(GLenum pname, GLuint buffer);
GLint GetUniformLocation(GLuint const pgm, char const *name);

#endif //DEMO_OPENGL_H_
