// This file was generated by a script @ 2018/03/04 19:02:34

#ifndef EXT_EXTENSIONS_H
#define EXT_EXTENSIONS_H

#include "GL/glext.h"

extern PFNGLATTACHSHADERPROC pfnAttachShader;
extern PFNGLBEGINQUERYPROC pfnBeginQuery;
extern PFNGLBINDBUFFERPROC pfnBindBuffer;
extern PFNGLBINDBUFFERBASEPROC pfnBindBufferBase;
extern PFNGLBINDBUFFERRANGEPROC pfnBindBufferRange;
extern PFNGLBINDBUFFERSBASEPROC pfnBindBuffersBase;
extern PFNGLBINDVERTEXARRAYPROC pfnBindVertexArray;
extern PFNGLBINDVERTEXBUFFERPROC pfnBindVertexBuffer;
extern PFNGLBLENDEQUATIONSEPARATEPROC pfnBlendEquationSeparate;
extern PFNGLBUFFERSTORAGEPROC pfnBufferStorage;
extern PFNGLBUFFERSUBDATAPROC pfnBufferSubData;
extern PFNGLCLEARNAMEDBUFFERSUBDATAPROC pfnClearNamedBufferSubData;
extern PFNGLCOMPILESHADERPROC pfnCompileShader;
extern PFNGLCOPYNAMEDBUFFERSUBDATAPROC pfnCopyNamedBufferSubData;
extern PFNGLCREATEPROGRAMPROC pfnCreateProgram;
extern PFNGLCREATEQUERIESPROC pfnCreateQueries;
extern PFNGLCREATESHADERPROC pfnCreateShader;
extern PFNGLCREATESHADERPROGRAMVPROC pfnCreateShaderProgramv;
extern PFNGLDELETEBUFFERSPROC pfnDeleteBuffers;
extern PFNGLDELETEPROGRAMPROC pfnDeleteProgram;
extern PFNGLDELETEQUERIESPROC pfnDeleteQueries;
extern PFNGLDELETESHADERPROC pfnDeleteShader;
extern PFNGLDELETEVERTEXARRAYSPROC pfnDeleteVertexArrays;
extern PFNGLDETACHSHADERPROC pfnDetachShader;
extern PFNGLDISPATCHCOMPUTEPROC pfnDispatchCompute;
extern PFNGLDISPATCHCOMPUTEINDIRECTPROC pfnDispatchComputeIndirect;
extern PFNGLDRAWARRAYSINDIRECTPROC pfnDrawArraysIndirect;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC pfnEnableVertexAttribArray;
extern PFNGLENDQUERYPROC pfnEndQuery;
extern PFNGLGENBUFFERSPROC pfnGenBuffers;
extern PFNGLGENVERTEXARRAYSPROC pfnGenVertexArrays;
extern PFNGLGENERATEMIPMAPPROC pfnGenerateMipmap;
extern PFNGLGETPROGRAMINFOLOGPROC pfnGetProgramInfoLog;
extern PFNGLGETPROGRAMRESOURCEINDEXPROC pfnGetProgramResourceIndex;
extern PFNGLGETPROGRAMIVPROC pfnGetProgramiv;
extern PFNGLGETQUERYOBJECTUIVPROC pfnGetQueryObjectuiv;
extern PFNGLGETSHADERINFOLOGPROC pfnGetShaderInfoLog;
extern PFNGLGETSHADERIVPROC pfnGetShaderiv;
extern PFNGLGETUNIFORMBLOCKINDEXPROC pfnGetUniformBlockIndex;
extern PFNGLGETUNIFORMLOCATIONPROC pfnGetUniformLocation;
extern PFNGLISPROGRAMPROC pfnIsProgram;
extern PFNGLLINKPROGRAMPROC pfnLinkProgram;
extern PFNGLMAPBUFFERPROC pfnMapBuffer;
extern PFNGLMAPBUFFERRANGEPROC pfnMapBufferRange;
extern PFNGLMAPNAMEDBUFFERRANGEPROC pfnMapNamedBufferRange;
extern PFNGLMEMORYBARRIERPROC pfnMemoryBarrier;
extern PFNGLNAMEDBUFFERSUBDATAPROC pfnNamedBufferSubData;
extern PFNGLPROGRAMUNIFORM1IPROC pfnProgramUniform1i;
extern PFNGLSHADERSOURCEPROC pfnShaderSource;
extern PFNGLSHADERSTORAGEBLOCKBINDINGPROC pfnShaderStorageBlockBinding;
extern PFNGLTEXSTORAGE2DPROC pfnTexStorage2D;
extern PFNGLTEXSTORAGE3DPROC pfnTexStorage3D;
extern PFNGLUNIFORM1FPROC pfnUniform1f;
extern PFNGLUNIFORM1IPROC pfnUniform1i;
extern PFNGLUNIFORM1UIPROC pfnUniform1ui;
extern PFNGLUNIFORM4FPROC pfnUniform4f;
extern PFNGLUNIFORM4FVPROC pfnUniform4fv;
extern PFNGLUNIFORMMATRIX3FVPROC pfnUniformMatrix3fv;
extern PFNGLUNIFORMMATRIX4FVPROC pfnUniformMatrix4fv;
extern PFNGLUNMAPBUFFERPROC pfnUnmapBuffer;
extern PFNGLUSEPROGRAMPROC pfnUseProgram;
extern PFNGLVALIDATEPROGRAMPROC pfnValidateProgram;
extern PFNGLVERTEXATTRIBBINDINGPROC pfnVertexAttribBinding;
extern PFNGLVERTEXATTRIBFORMATPROC pfnVertexAttribFormat;
extern PFNGLVERTEXATTRIBIFORMATPROC pfnVertexAttribIFormat;

#define glAttachShader pfnAttachShader
#define glBeginQuery pfnBeginQuery
#define glBindBuffer pfnBindBuffer
#define glBindBufferBase pfnBindBufferBase
#define glBindBufferRange pfnBindBufferRange
#define glBindBuffersBase pfnBindBuffersBase
#define glBindVertexArray pfnBindVertexArray
#define glBindVertexBuffer pfnBindVertexBuffer
#define glBlendEquationSeparate pfnBlendEquationSeparate
#define glBufferStorage pfnBufferStorage
#define glBufferSubData pfnBufferSubData
#define glClearNamedBufferSubData pfnClearNamedBufferSubData
#define glCompileShader pfnCompileShader
#define glCopyNamedBufferSubData pfnCopyNamedBufferSubData
#define glCreateProgram pfnCreateProgram
#define glCreateQueries pfnCreateQueries
#define glCreateShader pfnCreateShader
#define glCreateShaderProgramv pfnCreateShaderProgramv
#define glDeleteBuffers pfnDeleteBuffers
#define glDeleteProgram pfnDeleteProgram
#define glDeleteQueries pfnDeleteQueries
#define glDeleteShader pfnDeleteShader
#define glDeleteVertexArrays pfnDeleteVertexArrays
#define glDetachShader pfnDetachShader
#define glDispatchCompute pfnDispatchCompute
#define glDispatchComputeIndirect pfnDispatchComputeIndirect
#define glDrawArraysIndirect pfnDrawArraysIndirect
#define glEnableVertexAttribArray pfnEnableVertexAttribArray
#define glEndQuery pfnEndQuery
#define glGenBuffers pfnGenBuffers
#define glGenVertexArrays pfnGenVertexArrays
#define glGenerateMipmap pfnGenerateMipmap
#define glGetProgramInfoLog pfnGetProgramInfoLog
#define glGetProgramResourceIndex pfnGetProgramResourceIndex
#define glGetProgramiv pfnGetProgramiv
#define glGetQueryObjectuiv pfnGetQueryObjectuiv
#define glGetShaderInfoLog pfnGetShaderInfoLog
#define glGetShaderiv pfnGetShaderiv
#define glGetUniformBlockIndex pfnGetUniformBlockIndex
#define glGetUniformLocation pfnGetUniformLocation
#define glIsProgram pfnIsProgram
#define glLinkProgram pfnLinkProgram
#define glMapBuffer pfnMapBuffer
#define glMapBufferRange pfnMapBufferRange
#define glMapNamedBufferRange pfnMapNamedBufferRange
#define glMemoryBarrier pfnMemoryBarrier
#define glNamedBufferSubData pfnNamedBufferSubData
#define glProgramUniform1i pfnProgramUniform1i
#define glShaderSource pfnShaderSource
#define glShaderStorageBlockBinding pfnShaderStorageBlockBinding
#define glTexStorage2D pfnTexStorage2D
#define glTexStorage3D pfnTexStorage3D
#define glUniform1f pfnUniform1f
#define glUniform1i pfnUniform1i
#define glUniform1ui pfnUniform1ui
#define glUniform4f pfnUniform4f
#define glUniform4fv pfnUniform4fv
#define glUniformMatrix3fv pfnUniformMatrix3fv
#define glUniformMatrix4fv pfnUniformMatrix4fv
#define glUnmapBuffer pfnUnmapBuffer
#define glUseProgram pfnUseProgram
#define glValidateProgram pfnValidateProgram
#define glVertexAttribBinding pfnVertexAttribBinding
#define glVertexAttribFormat pfnVertexAttribFormat
#define glVertexAttribIFormat pfnVertexAttribIFormat

#endif  // EXT_EXTENSIONS_H
