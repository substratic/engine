#ifndef __subst_physics_h
#define __subst_physics_h

#include <mesche.h>

typedef struct {
  float center_x;
  float center_y;
  float center_z;
  float radius;
} SubstSphere;

void subst_physics_module_init(VM *vm);

#endif
