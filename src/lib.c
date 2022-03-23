#include "renderer.h"
#include "texture.h"
#include "window.h"

#include <mesche.h>

void substratic_library_init(VM *vm) {
  subst_window_module_init(vm);
  subst_texture_module_init(vm);
  subst_renderer_module_init(vm);
}
