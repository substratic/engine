#ifndef __subst_texture_h
#define __subst_texture_h

#include <inttypes.h>
#include <mesche.h>

typedef struct {
  uint32_t width;
  uint32_t height;
  uint32_t texture_id;
} SubstTexture;

typedef struct {
  bool use_smoothing;
} SubstTextureOptions;

SubstTexture *subst_texture_png_load(char *file_path,
                                     SubstTextureOptions *options);
void subst_texture_png_save(const char *file_path,
                            const unsigned char *image_data,
                            const uint32_t width, const uint32_t height);
Value subst_texture_func_image_load_internal(MescheMemory *mem, int arg_count,
                                             Value *args);

void subst_texture_module_init(VM *vm);
Value subst_texture_load_msc(MescheMemory *mem, int arg_count, Value *args);

#endif
