#ifndef __subst_font_h
#define __subst_font_h

#include "renderer.h"
#include <mesche.h>

typedef struct _SubstFont SubstFont;

extern SubstFont *subst_font_load_file(const char *font_path, int font_size);
extern void subst_font_render_text(SubstRenderer *renderer, SubstFont *font,
                                   const char *text, float pos_x, float pos_y);

// The returned string must be freed!
#ifndef __EMSCRIPTEN__
extern char *subst_font_resolve_path(const char *font_name);
#endif

Value subst_graphics_func_load_font_internal(MescheMemory *mem, int arg_count,
                                             Value *args);

void subst_font_module_init(VM *vm);

#endif
