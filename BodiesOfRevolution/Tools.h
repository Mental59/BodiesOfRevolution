#pragma once
#include <GL/glew.h>

GLuint createShader(const GLchar* code, GLenum type);

GLuint createProgram(GLuint vsh, GLuint fsh);
