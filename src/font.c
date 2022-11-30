#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#ifndef __EMSCRIPTEN__
#include <fontconfig/fontconfig.h>
#endif
#include <ft2build.h>
#include <glad/glad.h>
#include <inttypes.h>
#include <mesche.h>
#include <stdbool.h>
#include <string.h>
#include FT_FREETYPE_H

#include "font.h"
#include "log.h"
#include "renderer.h"
#include "shader.h"
#include "texture.h"

#define ASCII_CHAR_START 32
#define ASCII_CHAR_END 126

typedef struct {
  SubstTexture texture;
  int32_t bearing_x;
  int32_t bearing_y;
  uint32_t advance;
} SubstFontChar;

struct _SubstFont {
  SubstFontChar chars[ASCII_CHAR_END - ASCII_CHAR_START];
};

const char *FontVertexShaderText = GLSL(
    layout(location = 0) in vec2 position; layout(location = 1) in vec2 tex_uv;

    uniform mat4 model; uniform mat4 view; uniform mat4 projection;

    out vec2 tex_coords;

    void main() {
      tex_coords = tex_uv;
      gl_Position = projection * view * model * vec4(position, 0.0, 1.0);
    });

const char *FontFragmentShaderText =
    GLSL(in vec2 tex_coords;

         uniform sampler2D tex0; uniform vec4 color = vec4(1.0, 1.0, 1.0, 1.0);

         void main() {
           vec4 sampled = vec4(1.0, 1.0, 1.0, texture(tex0, tex_coords).r);
           gl_FragColor = color * sampled;
         });

SubstFont *subst_font_load_file(const char *font_path, int font_size) {
  char char_id = 0;
  SubstFontChar *current_char;

  static FT_Library ft;
  if (FT_Init_FreeType(&ft)) {
    subst_log("Could not load FreeType library\n");
    return NULL;
  }

  // Load the font file with FreeType
  FT_Face face;
  if (FT_New_Face(ft, font_path, 0, &face)) {
    subst_log("Failed to load font: %s\n", font_path);
    return NULL;
  }

  if (face == NULL) {
    subst_log("Could not load font: %s\n", font_path);
    return NULL;
  }

  // Specify the size of the face needed
  FT_Set_Pixel_Sizes(face, 0, font_size);

  // Initialize the font in memory
  SubstFont *subst_font = malloc(sizeof(struct _SubstFont));
  memset(subst_font, 0, sizeof(sizeof(struct _SubstFont)));

  // Remove alignment restriction
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  // Load glyphs for each character
  for (char_id = ASCII_CHAR_START; char_id < ASCII_CHAR_END; char_id++) {
    // Load the character glyph and information
    if (FT_Load_Char(face, char_id, FT_LOAD_RENDER)) {
      subst_log("Failed to load Glyph: %c\n", char_id);
      return NULL;
    }

    // Assign glyph metrics
    current_char = &subst_font->chars[char_id - ASCII_CHAR_START];
    current_char->texture.width = face->glyph->bitmap.width;
    current_char->texture.height = face->glyph->bitmap.rows;
    current_char->bearing_x = face->glyph->bitmap_left;
    current_char->bearing_y = face->glyph->bitmap_top;
    current_char->advance = face->glyph->advance.x;

    // Create the texture and copy the glyph bitmap into it
    glGenTextures(1, &current_char->texture.texture_id);
    glBindTexture(GL_TEXTURE_2D, current_char->texture.texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, current_char->texture.width,
                 current_char->texture.height, 0, GL_RED, GL_UNSIGNED_BYTE,
                 face->glyph->bitmap.buffer);

    // Set texture options to render the glyph correctly
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }

  // Free the face and the FreeType library
  FT_Done_Face(face);
  FT_Done_FreeType(ft);

  return subst_font;
}

void subst_font_render_text(SubstRenderer *renderer, SubstFont *font,
                            const char *text, float pos_x, float pos_y) {
  float x, y;
  uint8_t i = 0;
  uint8_t num_chars = 0;
  SubstFontChar *current_char = NULL;
  static SubstDrawArgs draw_args;

  if (draw_args.shader_program == 0) {
    const SubstShaderFile shader_files[] = {
        {GL_VERTEX_SHADER, FontVertexShaderText},
        {GL_FRAGMENT_SHADER, FontFragmentShaderText},
    };

    draw_args.shader_program = subst_shader_compile(shader_files, 2);
  }

  num_chars = strlen(text);

  for (i = 0; i < num_chars; i++) {
    // Get the char information
    current_char = font->chars + (text[i] - ASCII_CHAR_START);

    x = pos_x + current_char->bearing_x;
    y = pos_y - current_char->bearing_y;

    subst_renderer_draw_texture_ex(renderer, &current_char->texture, x, y,
                                   &draw_args);

    pos_x += current_char->advance >> 6;
  }

  glBindTexture(GL_TEXTURE_2D, 0);
}

int subst_font_text_width(SubstFont *font, const char *text) {
  float width = 0;
  uint8_t i = 0;
  uint8_t num_chars = strlen(text);
  SubstFontChar *current_char = NULL;

  for (i = 0; i < num_chars; i++) {
    // Get the char information
    current_char = font->chars + (text[i] - ASCII_CHAR_START);
    width += current_char->advance >> 6;
  }

  return width;
}

#ifndef __EMSCRIPTEN__

char *subst_font_resolve_path(const char *font_name) {
  char *font_path = NULL;

  // Initialize fontconfig library
  FcConfig *config = FcInitLoadConfigAndFonts();

  // Configure the search pattern
  FcPattern *pattern = FcNameParse((FcChar8 *)font_name);
  FcConfigSubstitute(config, pattern, FcMatchPattern);
  FcDefaultSubstitute(pattern);

  // Find a font that matches the query pattern
  FcResult result;
  FcPattern *font = FcFontMatch(config, pattern, &result);
  if (font) {
    FcChar8 *file_path = NULL;
    if (FcPatternGetString(font, FC_FILE, 0, &file_path) == FcResultMatch) {
      font_path = strdup((char *)file_path);
    }
  }

  FcPatternDestroy(font);
  FcPatternDestroy(pattern);
  FcConfigDestroy(config);

  return font_path;
}

void subst_font_print_all(const char *family_name) {
  // Initialize fontconfig library
  FcConfig *config = FcInitLoadConfigAndFonts();

  // Create a pattern to find all fonts
  FcPattern *pattern = NULL;
  if (family_name) {
    pattern = FcNameParse((FcChar8 *)family_name);
  } else {
    pattern = FcPatternCreate();
  }

  // Build a font set from the pattern
  FcObjectSet *object_set =
      FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_LANG, FC_FILE, (char *)0);
  FcFontSet *font_set = FcFontList(config, pattern, object_set);

  if (font_set) {
    subst_log("Font count: %d\n", font_set->nfont);
    for (int i = 0; font_set && i < font_set->nfont; ++i) {
      FcPattern *font = font_set->fonts[i];
      FcChar8 *file, *style, *family;
      if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch &&
          FcPatternGetString(font, FC_FAMILY, 0, &family) == FcResultMatch &&
          FcPatternGetString(font, FC_STYLE, 0, &style) == FcResultMatch) {
        subst_log("Font path: %s (family %s, style %s)\n", file, family, style);
      }
    }

    FcFontSetDestroy(font_set);
    FcObjectSetDestroy(object_set);
  } else {
    subst_log("No fonts found!\n");
  }

  if (pattern) {
    FcPatternDestroy(pattern);
  }

  FcConfigDestroy(config);
}

#endif

Value subst_font_load_msc(VM *vm, int arg_count, Value *args) {
#ifdef __EMSCRIPTEN__
  // TODO: Raise an error!
  return FALSE_VAL;
#else
  if (arg_count != 3) {
    subst_log("Function requires 3 parameters.");
  }

  SubstFont *font = NULL;

  char *family = AS_CSTRING(args[0]);
  char *weight = AS_CSTRING(args[1]);
  double size = AS_NUMBER(args[2]);

  char font_spec[100];

  sprintf(font_spec, "%s %s", family, weight);

  char *font_path = subst_font_resolve_path(font_spec);
  subst_log("Resolved font path: %s\n", font_path);
  if (!font_path) {
    subst_log("Could not find a file for font: %s\n", font_spec);
  } else {
    // Load the font and free the allocation font path
    font = subst_font_load_file(font_path, (int)size);
    free(font_path);
    font_path = NULL;
  }

  return OBJECT_VAL(mesche_object_make_pointer(vm, font, true));
#endif
}

Value subst_font_load_file_msc(VM *vm, int arg_count, Value *args) {
  if (arg_count != 2) {
    subst_log("Function requires 2 parameters.");
  }

  char *font_path = AS_CSTRING(args[0]);
  double size = AS_NUMBER(args[1]);

  SubstFont *font = subst_font_load_file(font_path, (int)size);
  return OBJECT_VAL(mesche_object_make_pointer(vm, font, true));
}

Value subst_font_text_width_msc(VM *vm, int arg_count, Value *args) {
  if (arg_count != 2) {
    subst_log("Function requires 2 parameters.");
  }

  SubstFont *font = AS_POINTER(args[0])->ptr;
  const char *text = AS_CSTRING(args[1]);

  return NUMBER_VAL(subst_font_text_width(font, text));
}

Value subst_font_height_msc(VM *vm, int arg_count, Value *args) {
  if (arg_count != 1) {
    subst_log("Function requires 1 parameter.");
  }

  SubstFont *font = AS_POINTER(args[0])->ptr;
  return NUMBER_VAL(font->chars['A' - ASCII_CHAR_START].bearing_y);
}

Value subst_font_render_text_msc(VM *vm, int arg_count, Value *args) {
  if (arg_count != 5) {
    subst_log("Function requires 5 parameters.");
  }

  SubstRenderer *renderer = AS_POINTER(args[0])->ptr;
  SubstFont *font = AS_POINTER(args[1])->ptr;
  const char *text = AS_CSTRING(args[2]);
  float pos_x = AS_NUMBER(args[3]);
  float pos_y = AS_NUMBER(args[4]);

  subst_font_render_text(renderer, font, text, pos_x, pos_y);

  return TRUE_VAL;
}

void subst_font_module_init(VM *vm) {
  mesche_vm_define_native_funcs(
      vm, "substratic font",
      (MescheNativeFuncDetails[]){
          {"font-load", subst_font_load_msc, true},
          {"font-load-file", subst_font_load_file_msc, true},
          {"font-text-width", subst_font_text_width_msc, true},
          {"font-height", subst_font_height_msc, true},
          {"render-text", subst_font_render_text_msc, true},
          {NULL, NULL, false}});
}
