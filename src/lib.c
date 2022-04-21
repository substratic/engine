#include "font.h"
#include "particle.h"
#include "physics.h"
#include "renderer.h"
#include "texture.h"
#include "window.h"

#include <mesche.h>

void substratic_library_init(VM *vm) {
  subst_font_module_init(vm);
  subst_input_module_init(vm);
  subst_window_module_init(vm);
  subst_texture_module_init(vm);
  subst_renderer_module_init(vm);
  subst_physics_module_init(vm);
  subst_particle_module_init(vm);
}
