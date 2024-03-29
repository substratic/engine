#ifndef __subst_window_h
#define __subst_window_h

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <mesche.h>

#include "input.h"

typedef struct {
  int width, height;
  bool is_resizing;
  GLFWwindow *glfwWindow;
  MescheMemory *mesche_mem;
  SubstInputState *input_state;
} SubstWindow;

SubstWindow *subst_window_create(int width, int height, const char *title);
void subst_window_show(SubstWindow *window);
void subst_window_destroy(SubstWindow *window);

void subst_window_module_init(VM *vm);
Value subst_window_create_msc(VM *vm, int arg_count, Value *args);
Value subst_window_show_msc(VM *vm, int arg_count, Value *args);
Value subst_window_needs_close_p_msc(VM *vm, int arg_count, Value *args);

#endif
