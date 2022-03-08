#ifndef __subst_shader_h
#define __subst_shader_h

#include <glad/glad.h>
#include <inttypes.h>

extern const char *DefaultVertexShaderText;
extern const char *DefaultFragmentShaderText;
extern const char *TexturedVertexShaderText;
extern const char *TexturedFragmentShaderText;

typedef struct {
  GLenum shader_type;
  const char *shader_text;
} SubstShaderFile;

GLuint subst_shader_compile(const SubstShaderFile *shader_files,
                            uint32_t shader_count);

#define GLSL(src) "#version 330 core\n" #src

#endif
