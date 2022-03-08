#include "shader.h"
#include "util.h"

#include <cglm/cglm.h>
#include <glad/glad.h>

const char *DefaultVertexShaderText =
    GLSL(layout(location = 0) in vec2 a_vec;

         uniform mat4 model; uniform mat4 view; uniform mat4 projection;

         void main() {
           gl_Position = projection * view * model * vec4(a_vec, 0.0, 1.0);
         });

const char *DefaultFragmentShaderText =
    GLSL(uniform vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
         void main() { gl_FragColor = color; });

const char *TexturedVertexShaderText = GLSL(
    layout(location = 0) in vec2 position; layout(location = 1) in vec2 tex_uv;

    uniform mat4 model; uniform mat4 view; uniform mat4 projection;

    out vec2 tex_coords;

    void main() {
      tex_coords = tex_uv;
      gl_Position = projection * view * model * vec4(position, 0.0, 1.0);
    });

const char *TexturedFragmentShaderText =
    GLSL(in vec2 tex_coords;

         uniform sampler2D tex0; uniform vec4 color = vec4(1.0, 1.0, 1.0, 1.0);

         void main() { gl_FragColor = texture(tex0, tex_coords) * color; });

GLuint subst_shader_compile(const SubstShaderFile *shader_files,
                            uint32_t shader_count) {
  GLuint shader_id = 0;
  GLuint shader_program;
  shader_program = glCreateProgram();

  // Compile the shader files
  for (GLuint i = 0; i < shader_count; i++) {
    shader_id = glCreateShader(shader_files[i].shader_type);
    glShaderSource(shader_id, 1, &shader_files[i].shader_text, NULL);
    glCompileShader(shader_id);

    int success = 0;
    char infoLog[512];
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
      glGetShaderInfoLog(shader_id, 512, NULL, infoLog);
      PANIC("Shader compilation failed:\n%s\n", infoLog);
    }

    glAttachShader(shader_program, shader_id);
  }

  // Link the full program
  glLinkProgram(shader_program);

  return shader_program;
}

void subst_graphics_shader_mat4_set(unsigned int shader_program_id,
                                    const char *uniform_name, mat4 matrix) {
  unsigned int uniformLoc =
      glGetUniformLocation(shader_program_id, uniform_name);
  if (uniformLoc == -1) {
    PANIC("Could not find shader matrix parameter: %s\n", uniform_name);
  }

  glUniformMatrix4fv(uniformLoc, 1, GL_FALSE, matrix[0]);
}
