#ifndef __subst_input_h
#define __subst_input_h

#include <mesche.h>

typedef enum {
  INPUT_EVENT_NONE,
  INPUT_EVENT_KEY_UP,
  INPUT_EVENT_KEY_DOWN,
  INPUT_EVENT_MOUSE_MOVE,
  INPUT_EVENT_MOUSE_SCROLL,
  INPUT_EVENT_MOUSE_BUTTON_UP,
  INPUT_EVENT_MOUSE_BUTTON_DOWN,
} SubstInputEventKind;

typedef struct SubstInputEvent {
  SubstInputEventKind kind;
  struct SubstInputEvent *next_event;
} SubstInputEvent;

typedef struct {
  SubstInputEvent event;
  int key_code;
  int modifiers;
} SubstInputKeyEvent;

typedef struct {
  SubstInputEvent event;
  double pos_x;
  double pos_y;
} SubstInputMouseMoveEvent;

typedef struct {
  SubstInputEvent event;
  double offset_x;
  double offset_y;
} SubstInputMouseScrollEvent;

typedef struct {
  SubstInputEvent event;
  int button;
  int modifiers;
} SubstInputMouseButtonEvent;

typedef struct {
  SubstInputEvent *first_event;
  SubstInputEvent *last_event;
} SubstInputState;

void subst_input_module_init(VM *vm);

#endif
