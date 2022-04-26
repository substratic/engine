#include <mesche.h>

#include "particle.h"
#include "renderer.h"

typedef struct {
  float min;
  float max;
} SubstParticleFactor;

typedef struct {
  float size;
  float pos_x, pos_y;
  float vel_x, vel_y;
  float time_left; // If this is 0, the particle is done
  SubstColor color;
} SubstParticle;

typedef struct {
  // Source Configuration
  int max_particles;
  float geometry;
  SubstParticleFactor size;
  SubstParticleFactor interval;
  SubstParticleFactor lifetime;
  SubstParticleFactor vel_x;
  SubstParticleFactor vel_y;
  SubstColor color;

  // Source State
  float next_particle_time;
  SubstParticle *particles;
  int num_particles;
  int oldest_particle;
} SubstParticleSource;

void particle_source_free_func(MescheMemory *mem, void *obj) {
  SubstParticleSource *source = (SubstParticleSource *)obj;
  free(source->particles);
  free(source);
}

const ObjectPointerType SubstParticleSourceType = {
    .name = "particle-source", .free_func = particle_source_free_func};

typedef struct {
  float current_time;
  float origin_x, origin_y;
  ObjectArray *sources;
} SubstParticleSystem;

void particle_system_mark_func(MescheMemory *mem, void *obj) {
  SubstParticleSystem *system = (SubstParticleSystem *)obj;
  mesche_mem_mark_object((VM *)mem, (Object *)system->sources);
}

const ObjectPointerType SubstParticleSystemType = {
    .name = "particle-system", .mark_func = particle_system_mark_func};

Value subst_particle_make_system_msc(MescheMemory *mem, int arg_count,
                                     Value *args) {
  SubstParticleSystem *system = malloc(sizeof(SubstParticleSystem));
  system->sources = mesche_object_make_array(
      (VM *)mem); // malloc(sizeof(SubstParticleSource *) * arg_count);
  system->current_time = 0;
  system->origin_x = 0;
  system->origin_y = 0;

  // Read in and process every particle source argument
  for (int i = 0; i < arg_count; i++) {
    mesche_value_array_write(mem, &system->sources->objects, args[i]);
  }

  return OBJECT_VAL(mesche_object_make_pointer_type((VM *)mem, system,
                                                    &SubstParticleSystemType));
}

void read_particle_factor(SubstParticleFactor *factor, Value *args,
                          int *index) {
  // TODO: Remove index pointer design, not needed since index always
  // increments by 1
  Value min = args[*index];
  if (IS_CONS(min)) {
    // Treat this as a range
    // TODO: Make sure we're dealing with a list of 2 numbers!
    factor->min = AS_NUMBER(AS_CONS(min)->car);
    factor->max = AS_NUMBER(AS_CONS(AS_CONS(min)->cdr)->car);
  } else if (IS_NUMBER(min)) {
    // Treat this as a single value
    factor->min = AS_NUMBER(min);
    factor->max = factor->min;
  }

  // Update the index so that the next read comes from the right place
  *index += 1;
}

Value subst_particle_make_source_msc(MescheMemory *mem, int arg_count,
                                     Value *args) {
  if (arg_count != 8) {
    subst_log("Function requires 8 parameters.");
  }

  // Create the particle source
  SubstParticleSource *source = malloc(sizeof(SubstParticleSource));
  source->max_particles = AS_NUMBER(args[0]);
  source->geometry = AS_NUMBER(args[1]);
  source->color = *((SubstColor *)AS_POINTER(args[2])->ptr);

  // Read in particle configuration factors
  int next_index = 3;
  read_particle_factor(&source->size, args, &next_index);
  read_particle_factor(&source->interval, args, &next_index);
  read_particle_factor(&source->lifetime, args, &next_index);
  read_particle_factor(&source->vel_x, args, &next_index);
  read_particle_factor(&source->vel_y, args, &next_index);

  // Allocate the particle array
  source->particles = malloc(sizeof(SubstParticle) * source->max_particles);
  source->num_particles = 0;
  source->oldest_particle = 0;

  /* (make-particle-source-internal max-particles geometry */
  /*                                color size */
  /*                                interval lifetime */
  /*                                vel-x vel-y) */

  return OBJECT_VAL(mesche_object_make_pointer_type((VM *)mem, source,
                                                    &SubstParticleSourceType));
}

double rand_double(double min, double max) {
  double range = max - min;
  double val = rand() % 100 / (double)100;
  return (range * val) + min;
}

double particle_factor_next_value(SubstParticleFactor *factor) {
  return rand_double(factor->min, factor->max);
}

SubstParticleSource *particle_system_source(SubstParticleSystem *system,
                                            int index) {
  return (SubstParticleSource
              *)(AS_POINTER(system->sources->objects.values[index])->ptr);
}

Value subst_particle_system_update_msc(MescheMemory *mem, int arg_count,
                                       Value *args) {
  SubstParticleSystem *system =
      ((SubstParticleSystem *)AS_POINTER(args[0])->ptr);
  double time_delta = AS_NUMBER(args[1]);

  // Update the current time for the system
  system->current_time += time_delta;

  // Update each of the sources
  for (int i = 0; i < system->sources->objects.count; i++) {
    // Has the creation interval passed?
    SubstParticleSource *source = particle_system_source(system, i);
    if (system->current_time >= source->next_particle_time) {
      // Initialize the new particle
      SubstParticle *particle = NULL;
      if (source->num_particles < source->max_particles) {
        // TODO: Factor in oldest_particle index!
        particle = &source->particles[source->num_particles];
        source->num_particles++;
      } else {
        particle = &source->particles[source->oldest_particle];
        source->oldest_particle++;
        if (source->oldest_particle == source->max_particles) {
          source->oldest_particle = 0;
        }
      }

      // Set particle factors
      particle->pos_x = system->origin_x + rand_double(0.0, source->geometry);
      particle->pos_y = system->origin_y + rand_double(0.0, source->geometry);
      particle->vel_x = particle_factor_next_value(&source->vel_x);
      particle->vel_y = particle_factor_next_value(&source->vel_y);
      particle->size = particle_factor_next_value(&source->size);
      particle->color = source->color; // TODO: Interpolate colors
      particle->time_left = particle_factor_next_value(&source->lifetime);

      // Decide the time for the next particle
      source->next_particle_time =
          system->current_time + particle_factor_next_value(&source->interval);
    }

    // Update all existing particles
    for (int j = 0; j < source->num_particles; j++) {
      // Factor in the oldest_particle index when looping
      SubstParticle *particle = &source->particles[j];

      // Update lifetime
      particle->time_left -= time_delta;
      if (particle->time_left > 0) {
        // Update position
        particle->pos_x += particle->vel_x * time_delta;
        particle->pos_y += particle->vel_y * time_delta;

        // TODO: Add fade
      }
    }
  }

  return T_VAL;
}

Value subst_particle_system_render_msc(MescheMemory *mem, int arg_count,
                                       Value *args) {
  SubstRenderer *renderer = ((SubstRenderer *)AS_POINTER(args[0])->ptr);
  SubstParticleSystem *system =
      ((SubstParticleSystem *)AS_POINTER(args[1])->ptr);

  // TODO: Use OpenGL instanced arrays for this instead!

  // Render each of the sources
  for (int i = 0; i < system->sources->objects.count; i++) {
    // Render each of the particles
    SubstParticleSource *source = particle_system_source(system, i);
    for (int j = 0; j < source->num_particles; j++) {
      // TODO: Offset based on oldest_particle
      SubstParticle *particle = &source->particles[j];
      if (particle->time_left > 0) {
        subst_renderer_draw_rect_fill(
            renderer, particle->pos_x, particle->pos_y, particle->size,
            particle->size, (float *)&particle->color);
      }
    }
  }

  return T_VAL;
}

Value subst_particle_system_origin_set_msc(MescheMemory *mem, int arg_count,
                                           Value *args) {
  if (arg_count != 3) {
    subst_log("Function requires 3 parameters.");
  }

  SubstParticleSystem *system =
      ((SubstParticleSystem *)AS_POINTER(args[0])->ptr);
  system->origin_x = AS_NUMBER(args[1]);
  system->origin_y = AS_NUMBER(args[2]);

  return T_VAL;
}

void subst_particle_module_init(VM *vm) {
  mesche_vm_define_native_funcs(
      vm, "substratic particle",
      (MescheNativeFuncDetails[]){
          {"make-particle-system", subst_particle_make_system_msc, true},
          {"make-particle-source-internal", subst_particle_make_source_msc,
           false},
          {"particle-system-update", subst_particle_system_update_msc, true},
          {"particle-system-render", subst_particle_system_render_msc, true},
          {"particle-system-origin-set!", subst_particle_system_origin_set_msc,
           true},
          {NULL, NULL, false}});
}
