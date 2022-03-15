#ifndef __subst_renderer_h
#define __subst_renderer_h

#include <cglm/cglm.h>
#include <glad/glad.h>
#include <mesche.h>

#include "shader.h"
#include "texture.h"
#include "window.h"

typedef struct {
  SubstWindow *window;
  vec2 screen_size;
  vec2 desired_size;
  mat4 screen_matrix;
  mat4 view_matrix;
} SubstRenderer;

typedef enum {
  SubstDrawNone,
  SubstDrawScaled = 1,
  SubstDrawRotated = 2,
  SubstDrawCentered = 4
} SubstDrawFlags;

typedef struct SubstDrawArgs {
  float scale_x, scale_y;
  float rotation;
  uint8_t flags;
  GLuint shader_program;
} SubstDrawArgs;

int subst_renderer_init(void);
void subst_renderer_end(void);

void subst_renderer_loop_start(SubstRenderer *renderer, MescheRepl *repl);

void subst_renderer_draw_args_scale(SubstDrawArgs *args, float scale_x,
                                    float scale_y);
void subst_renderer_draw_args_rotate(SubstDrawArgs *args, float rotation);
void subst_renderer_draw_args_center(SubstDrawArgs *args, bool centered);

/* void subst_renderer_draw_texture(SubstRenderContext *context, */
/*                                  SubstTexture *texture, float x, float y); */
/* void subst_renderer_draw_texture_ex(SubstRenderContext *context, */
/*                                     SubstTexture *texture, float x, float y,
 */
/*                                     SubstDrawArgs *args); */

void subst_renderer_draw_rect_fill(SubstRenderer *renderer, float x, float y,
                                   float w, float h, vec4 color);

void subst_renderer_module_init(VM *vm);
Value subst_renderer_create_msc(MescheMemory *mem, int arg_count, Value *args);
Value subst_renderer_clear_msc(MescheMemory *mem, int arg_count, Value *args);
Value subst_renderer_swap_buffers_msc(MescheMemory *mem, int arg_count,
                                      Value *args);

Value subst_renderer_func_render_to_file(MescheMemory *mem, int arg_count,
                                         Value *args);
Value subst_renderer_draw_texture_msc(MescheMemory *mem, int arg_count,
                                      Value *args);

#endif
