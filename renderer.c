#include "log.h"
#include "renderer.h"
#include "util.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <glad/glad.h>
#include <inttypes.h>
#include <stdlib.h>

uint8_t subst_renderer_initialized = 0;

static char output_image_path[1024];

static void subst_renderer_window_size_update(SubstRenderer *renderer,
                                              int width, int height) {
  // Get the current framebuffer size
  glfwGetFramebufferSize(renderer->window->glfwWindow, &width, &height);

  renderer->window->width = width;
  renderer->window->height = height;
  renderer->screen_size[0] = width;
  renderer->screen_size[1] = height;

  // Update the viewport and recalculate projection matrix
  glViewport(0, 0, width, height);
  glm_ortho(0.f, renderer->screen_size[0], renderer->screen_size[1], 0.f, -1.f,
            1.f, renderer->screen_matrix);

  // Window is no longer resizing
  renderer->window->is_resizing = false;
}

static void subst_renderer_window_size_callback(GLFWwindow *glfwWindow,
                                                int width, int height) {
  SubstRenderer *renderer;

  subst_log("Window size changed: %dx%d\n", width, height);

  renderer = glfwGetWindowUserPointer(glfwWindow);

  if (!renderer) {
    subst_log("Cannot get SubstRenderer from GLFWwindow!\n");
    return;
  }

  // Update the screen size and projection matrix
  subst_renderer_window_size_update(renderer, width, height);
}

SubstRenderer *subst_renderer_create(SubstWindow *window) {
  SubstRenderer *renderer = malloc(sizeof(SubstRenderer));

  if (!window) {
    PANIC("Window was not initialized!\n");
  }

  // Initialize the renderer
  renderer->window = window;
  renderer->screen_size[0] = window->width;
  renderer->screen_size[1] = window->height;

  // Set up the default view matrix
  glm_mat4_identity(renderer->view_matrix);
  /* glm_scale(renderer->view_matrix, (vec3){1.f, 1.f, 1.f}); */

  // Set the "user pointer" of the GLFW window to our renderer
  glfwSetWindowUserPointer(window->glfwWindow, renderer);

  // Respond to window size changes
  glfwSetWindowSizeCallback(window->glfwWindow,
                            subst_renderer_window_size_callback);

  // Make the window's context current before loading OpenGL DLLs
  glfwMakeContextCurrent(window->glfwWindow);

  // Bind to OpenGL functions
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    PANIC("Failed to initialize GLAD!");
    return NULL;
  }

  subst_log("OpenGL Version %d.%d loaded\n", GLVersion.major, GLVersion.minor);

  return renderer;
}

void subst_renderer_draw_rect_fill(SubstRenderer *renderer, float x, float y,
                                   float w, float h, vec4 color) {
  static GLuint shader_program = 0;
  static GLuint rect_vertex_array = 0;
  static GLuint rect_vertex_buffer = 0;
  static GLuint rect_element_buffer = 0;

  if (shader_program == 0) {
    const SubstShaderFile shader_files[] = {
        {GL_VERTEX_SHADER, DefaultVertexShaderText},
        {GL_FRAGMENT_SHADER, DefaultFragmentShaderText},
    };
    shader_program = subst_shader_compile(shader_files, 2);
  }

  // Use the shader
  glUseProgram(shader_program);

  if (rect_vertex_array == 0) {
    glGenVertexArrays(1, &rect_vertex_array);
    glGenBuffers(1, &rect_vertex_buffer);
    glGenBuffers(1, &rect_element_buffer);

    float vertices[] = {
        // Positions
        0.5f,  0.5f,  // top right
        0.5f,  -0.5f, // bottom right
        -0.5f, -0.5f, // bottom left
        -0.5f, 0.5f,  // top left
    };

    unsigned int indices[] = {
        0, 1, 2, // first triangle
        2, 3, 0  // second triangle
    };

    glBindVertexArray(rect_vertex_array);

    glBindBuffer(GL_ARRAY_BUFFER, rect_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rect_element_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
  } else {
    glBindVertexArray(rect_vertex_array);
  }

  // Model matrix is scaled to size and translated
  mat4 model;
  glm_translate_make(model, (vec3){x + (w / 2.f), y + (h / 2.f), 0.f});
  glm_scale(model, (vec3){w, h, 0.f});

  // Set the uniforms
  glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1,
                     GL_FALSE, (float *)renderer->screen_matrix);
  glUniformMatrix4fv(glGetUniformLocation(shader_program, "view"), 1, GL_FALSE,
                     (float *)renderer->view_matrix);
  glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE,
                     (float *)model);
  glUniform4fv(glGetUniformLocation(shader_program, "color"), 1,
               (float *)color);

  // Draw all 6 indices in the element buffer
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void subst_renderer_draw_args_init(SubstDrawArgs *args, float scale) {
  args->shader_program = 0;
  if (scale != 0) {
    args->scale_x = scale;
    args->scale_y = scale;
    args->flags |= SubstDrawScaled;
  }
}

void subst_renderer_draw_args_scale(SubstDrawArgs *args, float scale_x,
                                    float scale_y) {
  if (scale_x != 0 || scale_y != 0) {
    args->scale_x = scale_x;
    args->scale_y = scale_y;
    args->flags |= SubstDrawScaled;
  }
}

void subst_renderer_draw_args_rotate(SubstDrawArgs *args, float rotation) {
  if (rotation != 0.0) {
    args->rotation = rotation;
    args->flags |= SubstDrawRotated;
  }
}

void subst_renderer_draw_args_center(SubstDrawArgs *args, bool centered) {
  if (centered) {
    args->flags |= SubstDrawCentered;
  }
}

void subst_renderer_draw_texture_ex(SubstRenderer *renderer,
                                    SubstTexture *texture, float x, float y,
                                    SubstDrawArgs *args) {
  GLuint shader_program = 0;
  static GLuint default_shader_program = 0;
  static GLuint rect_vertex_array = 0;
  static GLuint rect_vertex_buffer = 0;
  static GLuint rect_element_buffer = 0;

  if (args != NULL) {
    shader_program = args->shader_program;
  }

  // Use the default texture shader if one isn't specified
  if (shader_program == 0) {
    if (default_shader_program == 0) {
      const SubstShaderFile shader_files[] = {
          {GL_VERTEX_SHADER, TexturedVertexShaderText},
          {GL_FRAGMENT_SHADER, TexturedFragmentShaderText},
      };
      default_shader_program = subst_shader_compile(shader_files, 2);
    }

    shader_program = default_shader_program;
  }

  // Use the shader
  glUseProgram(shader_program);

  if (rect_vertex_array == 0) {
    glGenVertexArrays(1, &rect_vertex_array);
    glGenBuffers(1, &rect_vertex_buffer);
    glGenBuffers(1, &rect_element_buffer);

    // Texture coordinates are 0,0 for bottom left and 1,1 for top right
    float vertices[] = {
        // Positions   // Texture
        0.5f,  0.5f,  1.f, 1.f, // top right
        0.5f,  -0.5f, 1.f, 0.f, // bottom right
        -0.5f, -0.5f, 0.f, 0.f, // bottom left
        -0.5f, 0.5f,  0.f, 1.f, // top left
    };

    unsigned int indices[] = {
        0, 1, 2, // first triangle
        2, 3, 0  // second triangle
    };

    glBindVertexArray(rect_vertex_array);

    glBindBuffer(GL_ARRAY_BUFFER, rect_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rect_element_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (const void *)(2 * sizeof(float)));
  } else {
    glBindVertexArray(rect_vertex_array);
  }

  // Adjust position if texture shouldn't be drawn centered
  if (args && (args->flags & SubstDrawCentered) == 0) {
    x += texture->width / 2.f;
    y += texture->height / 2.f;
  }

  float scale_x = 1.f, scale_y = 1.f;
  if (args && (args->flags & SubstDrawScaled) == SubstDrawScaled) {
    scale_x = args->scale_x;
    scale_y = args->scale_y;
  }

  // Model matrix is scaled to size and translated
  mat4 model;
  glm_translate_make(model, (vec3){x * scale_x, y * scale_y, 0.f});
  glm_scale(model,
            (vec3){texture->width * scale_x, texture->height * scale_y, 0.f});

  // Rotate and scale as requested
  if (args && (args->flags & SubstDrawRotated) == SubstDrawRotated) {
    glm_rotate(model, glm_rad(args->rotation), (vec3){0.f, 0.f, 1.f});
  }

  mat4 view;
  glm_mat4_identity(view);

  // Set the uniforms
  vec4 color = {1.f, 1.f, 1.f, 1.f};
  glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1,
                     GL_FALSE, (float *)renderer->screen_matrix);
  glUniformMatrix4fv(glGetUniformLocation(shader_program, "view"), 1, GL_FALSE,
                     (float *)renderer->view_matrix);
  glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE,
                     (float *)model);
  glUniform4fv(glGetUniformLocation(shader_program, "color"), 1,
               (float *)color);

  // Bind the texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture->texture_id);
  glUniform1i(glGetUniformLocation(shader_program, "tex0"), 0);

  // Draw all 6 indices in the element buffer
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  // Reset the texture
  glBindTexture(GL_TEXTURE_2D, 0);
}

void subst_renderer_draw_texture(SubstRenderer *renderer, SubstTexture *texture,
                                 float x, float y) {
  subst_renderer_draw_texture_ex(renderer, texture, x, y, NULL);
}

void subst_renderer_save_to_png(SubstRenderer *renderer,
                                const char *output_file_path) {
  int i = 0;
  unsigned char *screen_bytes = NULL;
  unsigned char *image_bytes = NULL;
  size_t image_row_length = 4 * renderer->window->width;
  size_t image_data_size =
      sizeof(*image_bytes) * image_row_length * renderer->window->height;

  /* subst_log("Rendering window of size %u / %u to file: %s\n",
   * renderer->window->width, renderer->window->height, */
  /*          output_file_path); */

  // Allocate storage for the screen bytes
  // TODO: reduce memory allocation requirements
  screen_bytes = malloc(image_data_size);
  image_bytes = malloc(image_data_size);

  // TODO: Switch context to this window

  // Store the screen contents to a byte array
  glReadPixels(0, 0, renderer->window->width, renderer->window->height, GL_RGBA,
               GL_UNSIGNED_BYTE, screen_bytes);

  // Flip the rows of the byte array because OpenGL's coordinate system is
  // flipped
  for (i = 0; i < renderer->window->height; i++) {
    memcpy(&image_bytes[image_row_length * i],
           &screen_bytes[image_row_length *
                         ((int)renderer->window->height - (i + 1))],
           sizeof(*image_bytes) * image_row_length);
  }

  // Save image data to a PNG file
  subst_texture_png_save(output_file_path, image_bytes, renderer->window->width,
                         renderer->window->height);

  // Clean up the memory
  free(image_bytes);
  free(screen_bytes);
}

void subst_renderer_end(void) { glfwTerminate(); }

Value subst_renderer_func_render_to_file(MescheMemory *mem, int arg_count,
                                         Value *args) {
  if (arg_count != 1) {
    subst_log("Function requires a path to the output file.");
  }

  char *file_path = AS_CSTRING(args[0]);
  subst_log("Received request to save image: %s\n", file_path);
  memcpy(output_image_path, file_path, strlen(file_path));
}

void subst_renderer_module_init(VM *vm) {
  mesche_vm_define_native_funcs(
      vm, "substratic renderer",
      &(MescheNativeFuncDetails[]){
          {"renderer-create", subst_renderer_create_msc, true},
          {"renderer-clear", subst_renderer_clear_msc, true},
          {"renderer-swap-buffers", subst_renderer_swap_buffers_msc, true},
          {"renderer-draw-texture", subst_renderer_draw_texture_msc, true},
          {NULL, NULL, false}});
}

Value subst_renderer_create_msc(MescheMemory *mem, int arg_count, Value *args) {
  if (arg_count != 1) {
    subst_log("Function requires 1 parameter.");
  }

  ObjectPointer *ptr = AS_POINTER(args[0]);
  SubstWindow *window = (SubstWindow *)ptr->ptr;

  // Create the renderer
  SubstRenderer *renderer = subst_renderer_create(window);

  return OBJECT_VAL(mesche_object_make_pointer((VM *)mem, renderer, true));
}

Value subst_renderer_clear_msc(MescheMemory *mem, int arg_count, Value *args) {
  if (arg_count != 4) {
    subst_log("Function requires 4 parameters.");
  }

  ObjectPointer *ptr = AS_POINTER(args[0]);
  SubstRenderer *renderer = (SubstRenderer *)ptr->ptr;
  int r = AS_NUMBER(args[1]);
  int g = AS_NUMBER(args[2]);
  int b = AS_NUMBER(args[3]);

  glClearColor(r / 255.0, g / 255.0, b / 255.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  return T_VAL;
}

Value subst_renderer_swap_buffers_msc(MescheMemory *mem, int arg_count,
                                      Value *args) {
  ObjectPointer *ptr = AS_POINTER(args[0]);
  SubstRenderer *renderer = (SubstRenderer *)ptr->ptr;

  // Swap the render buffers
  glfwSwapBuffers(renderer->window->glfwWindow);

  return T_VAL;
}

Value subst_renderer_draw_texture_msc(MescheMemory *mem, int arg_count,
                                      Value *args) {
  ObjectPointer *ptr = AS_POINTER(args[0]);
  SubstRenderer *renderer = (SubstRenderer *)ptr->ptr;
  SubstTexture *texture = (SubstTexture *)AS_POINTER(args[1])->ptr;
  int x = AS_NUMBER(args[2]);
  int y = AS_NUMBER(args[3]);

  SubstDrawArgs draw_args;
  subst_renderer_draw_args_init(&draw_args, 2.0);
  subst_renderer_draw_texture_ex(renderer, texture, x, y, &draw_args);

  return T_VAL;
}
