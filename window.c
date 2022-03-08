#include "window.h"
#include <glad/glad.h>

static void glfw_error_callback(int error, const char *description) {
  subst_log("GLFW error %d: %s\n", error, description);
}

SubstWindow *subst_window_create(int width, int height, const char *title) {
  SubstWindow *window;
  GLFWwindow *glfwWindow;

  if (!glfwInit()) {
    subst_log("GLFW failed to init!\n");
    return NULL;
  }

  // Set OpenGL version and profile
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

#ifdef __APPLE__
  // Just in case...
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  // Make sure all new windows are hidden by default
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

  // Make the window float
  glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  // Enable anti-aliasing
  glfwWindowHint(GLFW_SAMPLES, 4);

  printf("ABOUT TO CREATE WINDOW: %d %d\n", width, height);

  // Make sure we're notified about errors
  glfwSetErrorCallback(glfw_error_callback);

  // Create the window
  glfwWindow = glfwCreateWindow(width, height, title, NULL, NULL);
  if (!glfwWindow) {
    subst_log("Could not create GLFW window!\n");
    return NULL;
  }

  window = malloc(sizeof(SubstWindow));
  window->glfwWindow = glfwWindow;
  window->width = width;
  window->height = width;
  window->is_resizing = false;

  return window;
}

void subst_window_size_set(SubstWindow *window, int width, int height) {
  window->is_resizing = true;
  glfwSetWindowSize(window->glfwWindow, width, height);
}

void subst_window_show(SubstWindow *window) {
  if (window && window->glfwWindow) {
    glfwShowWindow(window->glfwWindow);

    // TODO: Is there a more appropriate place for these?

    // Set the swap interval to prevent tearing
    glfwSwapInterval(1);

    // Enable blending
    glEnable(GL_BLEND);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

    // Enable textures
    glEnable(GL_TEXTURE_2D);

    // Enable multisampling (anti-aliasing)
    /* glEnable(GL_MULTISAMPLE); */
  }
}

void subst_window_destroy(SubstWindow *window) {
  if (window != NULL) {
    glfwDestroyWindow(window->glfwWindow);
    window->glfwWindow = NULL;
    free(window);
  }
}

void subst_window_module_init(VM *vm) {
  mesche_vm_define_native_funcs(
      vm, "substratic window",
      &(MescheNativeFuncDetails[]){
          {"window-create", subst_window_create_msc, true},
          {"window-show", subst_window_show_msc, true},
          {"window-needs-close?", subst_window_needs_close_p_msc, true},
          {NULL, NULL, false}});
}

Value subst_window_create_msc(MescheMemory *mem, int arg_count, Value *args) {
  if (arg_count != 3) {
    subst_log("Function requires 3 number parameters.");
  }

  int width = AS_NUMBER(args[0]);
  int height = AS_NUMBER(args[1]);
  const char *title = AS_CSTRING(args[2]);

  // Create the window
  SubstWindow *window = subst_window_create(width, height, title);

  return OBJECT_VAL(mesche_object_make_pointer((VM *)mem, window, true));
}

Value subst_window_show_msc(MescheMemory *mem, int arg_count, Value *args) {
  ObjectPointer *ptr = AS_POINTER(args[0]);
  subst_window_show((SubstWindow *)ptr->ptr);

  return T_VAL;
}

Value subst_window_needs_close_p_msc(MescheMemory *mem, int arg_count,
                                     Value *args) {
  ObjectPointer *ptr = AS_POINTER(args[0]);
  glfwPollEvents();
  SubstWindow *window = (SubstWindow *)ptr->ptr;
  return glfwWindowShouldClose(window->glfwWindow) ? T_VAL : NIL_VAL;
}
