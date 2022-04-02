#include <cglm/cglm.h>

#include "physics.h"

Value physics_make_sphere_msc(MescheMemory *mem, int arg_count, Value *args) {
  SubstSphere *sphere = malloc(sizeof(SubstSphere));
  sphere->center_x = AS_NUMBER(args[0]);
  sphere->center_y = AS_NUMBER(args[1]);
  sphere->center_z = 0.0;
  sphere->radius = AS_NUMBER(args[2]);

  return OBJECT_VAL(mesche_object_make_pointer((VM *)mem, sphere, true));
}

Value physics_sphere_intersect_msc(MescheMemory *mem, int arg_count,
                                   Value *args) {
  SubstSphere *sphere1 = (SubstSphere *)AS_POINTER(args[0])->ptr;
  SubstSphere *sphere2 = (SubstSphere *)AS_POINTER(args[1])->ptr;
  return BOOL_VAL(glm_sphere_sphere((float *)sphere1, (float *)sphere2));
}

Value physics_sphere_center_x_set_msc(MescheMemory *mem, int arg_count,
                                      Value *args) {
  SubstSphere *sphere = (SubstSphere *)AS_POINTER(args[0])->ptr;
  sphere->center_x = AS_NUMBER(args[1]);
  return args[1];
}

Value physics_sphere_center_y_set_msc(MescheMemory *mem, int arg_count,
                                      Value *args) {
  SubstSphere *sphere = (SubstSphere *)AS_POINTER(args[0])->ptr;
  sphere->center_y = AS_NUMBER(args[1]);
  return args[1];
}

void subst_physics_module_init(VM *vm) {
  mesche_vm_define_native_funcs(
      vm, "substratic physics",
      (MescheNativeFuncDetails[]){
          {"make-sphere", physics_make_sphere_msc, true},
          {"sphere-intersect?", physics_sphere_intersect_msc, true},
          {"sphere-center-x-set!", physics_sphere_center_x_set_msc, true},
          {"sphere-center-y-set!", physics_sphere_center_y_set_msc, true},
          {NULL, NULL, false}});
}
