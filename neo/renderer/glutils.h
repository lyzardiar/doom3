#ifndef __GLUTILS_H__
#define __GLUTILS_H__

void GL_CheckError(const char* op);

GLuint GL_CreateProgram(const char* pVertexSource, const char* pFragmentSource);
GLuint GL_CreateProgramFromFile(const char* vert, const char* frag);
GLuint GL_CompileShader(GLenum shaderType, const char* source);
GLuint GL_CompileShaderFromFile(GLenum target, const char* filename);
GLuint GL_LinkProgram(GLuint program, GLuint vert, GLuint frag);

#endif
