#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <stdlib.h>

#include "input.h"
#include "renderer.h"
#include "window.h"

Value input_event_create_pointer(MescheMemory *mem, SubstInputEvent event) {
  SubstInputEvent *event_ptr = malloc(sizeof(SubstInputEvent));
  memcpy(event_ptr, &event, 1);
  return OBJECT_VAL(mesche_object_make_pointer((VM *)mem, event_ptr, true));
}

void input_event_push(SubstInputState *input_state,
                      SubstInputEvent *input_event) {
  // Push the event into the linked list
  if (input_state->last_event == NULL) {
    input_state->first_event = input_event;
    input_state->last_event = input_event;
  } else {
    input_state->last_event->next_event = input_event;
    input_state->last_event = input_event;
  }
}

void input_key_callback(GLFWwindow *window, int key, int scancode, int action,
                        int mods) {
  // TODO: Don't have SubstRenderer be the top-level engine state type
  SubstRenderer *renderer = (SubstRenderer *)glfwGetWindowUserPointer(window);
  SubstInputState *input_state = renderer->window->input_state;

  // TODO: We currently don't support key repeat events
  if (action != GLFW_REPEAT) {
    SubstInputKeyEvent *input_event = malloc(sizeof(SubstInputKeyEvent));
    input_event->event.kind =
        action == GLFW_PRESS ? INPUT_EVENT_KEY_DOWN : INPUT_EVENT_KEY_UP;
    input_event->event.next_event = NULL;
    input_event->key_code = key;
    input_event->key_modifiers = mods;

    input_event_push(input_state, input_event);
  }
}

void input_cursor_position_callback(GLFWwindow *window, double pos_x,
                                    double pos_y) {
  // TODO: Don't have SubstRenderer be the top-level engine state type
  SubstRenderer *renderer = (SubstRenderer *)glfwGetWindowUserPointer(window);
  SubstInputState *input_state = renderer->window->input_state;

  SubstInputMouseMoveEvent *input_event =
      malloc(sizeof(SubstInputMouseMoveEvent));
  input_event->event.kind = INPUT_EVENT_MOUSE_MOVE;
  input_event->event.next_event = NULL;
  input_event->pos_x = pos_x;
  input_event->pos_y = pos_y;

  input_event_push(input_state, input_event);
}

Value input_init_msc(MescheMemory *mem, int arg_count, Value *args) {
  ObjectPointer *ptr = AS_POINTER(args[0]);
  SubstWindow *window = (SubstWindow *)ptr->ptr;

  SubstInputState *input_state = malloc(sizeof(SubstInputState));
  input_state->first_event = NULL;
  input_state->last_event = NULL;

  // Set up GLFW input callbacks
  window->input_state = input_state;
  glfwSetKeyCallback(window->glfwWindow, input_key_callback);
  glfwSetCursorPosCallback(window->glfwWindow, input_cursor_position_callback);

  return OBJECT_VAL(mesche_object_make_pointer((VM *)mem, input_state, true));
}

Value input_event_peek_msc(MescheMemory *mem, int arg_count, Value *args) {
  ObjectPointer *ptr = AS_POINTER(args[0]);
  SubstInputState *input_state = (SubstInputState *)ptr->ptr;

  // Take the first event, remove it from the linked list
  SubstInputEvent *input_event = input_state->first_event;
  if (input_event != NULL) {
    return OBJECT_VAL(
        mesche_object_make_pointer((VM *)mem, input_event, false));
  } else {
    return NIL_VAL;
  }
}

Value input_event_take_msc(MescheMemory *mem, int arg_count, Value *args) {
  ObjectPointer *ptr = AS_POINTER(args[0]);
  SubstInputState *input_state = (SubstInputState *)ptr->ptr;

  // Take the first event, remove it from the linked list
  SubstInputEvent *input_event = input_state->first_event;
  if (input_event != NULL) {
    // Make sure to clear out the last event if we have no more events
    input_state->first_event = input_event->next_event;
    if (input_state->first_event == NULL) {
      input_state->last_event = NULL;
    }

    return OBJECT_VAL(mesche_object_make_pointer((VM *)mem, input_event, true));
  } else {
    return NIL_VAL;
  }
}

Value input_event_key_code_msc(MescheMemory *mem, int arg_count, Value *args) {
  ObjectPointer *ptr = AS_POINTER(args[0]);
  SubstInputKeyEvent *input_event = (SubstInputKeyEvent *)ptr->ptr;
  return NUMBER_VAL(input_event->key_code);
}

Value input_event_key_down_msc(MescheMemory *mem, int arg_count, Value *args) {
  ObjectPointer *ptr = AS_POINTER(args[0]);
  SubstInputKeyEvent *input_event = (SubstInputKeyEvent *)ptr->ptr;
  return BOOL_VAL(input_event->event.kind == INPUT_EVENT_KEY_DOWN);
}

Value input_event_key_up_msc(MescheMemory *mem, int arg_count, Value *args) {
  ObjectPointer *ptr = AS_POINTER(args[0]);
  SubstInputKeyEvent *input_event = (SubstInputKeyEvent *)ptr->ptr;
  return BOOL_VAL(input_event->event.kind == INPUT_EVENT_KEY_UP);
}

Value input_event_mouse_x_msc(MescheMemory *mem, int arg_count, Value *args) {
  // TODO: Check event kind!
  ObjectPointer *ptr = AS_POINTER(args[0]);
  SubstInputMouseMoveEvent *input_event = (SubstInputMouseMoveEvent *)ptr->ptr;
  return NUMBER_VAL(input_event->pos_x);
}

Value input_event_mouse_y_msc(MescheMemory *mem, int arg_count, Value *args) {
  // TODO: Check event kind!
  ObjectPointer *ptr = AS_POINTER(args[0]);
  SubstInputMouseMoveEvent *input_event = (SubstInputMouseMoveEvent *)ptr->ptr;
  return NUMBER_VAL(input_event->pos_y);
}

Value input_event_mouse_move_msc(MescheMemory *mem, int arg_count,
                                 Value *args) {
  ObjectPointer *ptr = AS_POINTER(args[0]);
  SubstInputMouseMoveEvent *input_event = (SubstInputMouseMoveEvent *)ptr->ptr;
  return BOOL_VAL(input_event->event.kind == INPUT_EVENT_MOUSE_MOVE);
}

void subst_input_module_init(VM *vm) {
  mesche_vm_define_native_funcs(
      vm, "substratic input",
      (MescheNativeFuncDetails[]){
          {"input-init", input_init_msc, true},
          /* {"input-update", input_update_msc, true}, */
          {"input-event-peek", input_event_peek_msc, true},
          {"input-event-take", input_event_take_msc, true},

          // Keyboard events
          {"input-event-key-code", input_event_key_code_msc, true},
          {"input-event-key-down?", input_event_key_down_msc, true},
          {"input-event-key-up?", input_event_key_up_msc, true},

          // Mouse events
          {"input-event-mouse-x", input_event_mouse_x_msc, true},
          {"input-event-mouse-y", input_event_mouse_y_msc, true},
          {"input-event-mouse-move?", input_event_mouse_move_msc, true},
          {NULL, NULL, false}});
}
