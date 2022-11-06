#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <mesche.h>
#include <stdlib.h>

#include "input.h"
#include "renderer.h"
#include "window.h"

void input_free_func(MescheMemory *mem, void *obj) {
  SubstInputState *input_state = (SubstInputState *)obj;

  // Free all remaining events
  SubstInputEvent *event = input_state->first_event;
  while (event != NULL) {
    SubstInputEvent *this = event;
    event = event->next_event;
    free(this);
  }

  free(input_state);
}

void input_event_free_func(MescheMemory *mem, void *obj) {
  SubstInputEvent *input_event = (SubstInputEvent *)obj;
  free(input_event);
}

const ObjectPointerType SubstInputStateType = {.name = "input-state",
                                               .free_func = input_free_func};

const ObjectPointerType SubstInputEventType = {
    .name = "input-event", .free_func = input_event_free_func};

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

    input_event_push(input_state, (SubstInputEvent *)input_event);
  }
}

void input_mouse_button_callback(GLFWwindow *window, int button, int action,
                                 int mods) {
  // TODO: Don't have SubstRenderer be the top-level engine state type
  SubstRenderer *renderer = (SubstRenderer *)glfwGetWindowUserPointer(window);
  SubstInputState *input_state = renderer->window->input_state;

  SubstInputMouseButtonEvent *input_event =
      malloc(sizeof(SubstInputMouseButtonEvent));
  input_event->event.kind = action == GLFW_PRESS ? INPUT_EVENT_MOUSE_BUTTON_DOWN
                                                 : INPUT_EVENT_MOUSE_BUTTON_UP;
  input_event->event.next_event = NULL;
  input_event->button = button;
  input_event->modifiers = mods;

  input_event_push(input_state, (SubstInputEvent *)input_event);
}

void input_mouse_position_callback(GLFWwindow *window, double pos_x,
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

  input_event_push(input_state, (SubstInputEvent *)input_event);
}

void input_mouse_scroll_callback(GLFWwindow *window, double offset_x,
                                 double offset_y) {
  // TODO: Don't have SubstRenderer be the top-level engine state type
  SubstRenderer *renderer = (SubstRenderer *)glfwGetWindowUserPointer(window);
  SubstInputState *input_state = renderer->window->input_state;

  SubstInputMouseScrollEvent *input_event =
      malloc(sizeof(SubstInputMouseScrollEvent));
  input_event->event.kind = INPUT_EVENT_MOUSE_SCROLL;
  input_event->event.next_event = NULL;
  input_event->offset_x = offset_x;
  input_event->offset_y = offset_y;

  input_event_push(input_state, (SubstInputEvent *)input_event);
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
  glfwSetScrollCallback(window->glfwWindow, input_mouse_scroll_callback);
  glfwSetMouseButtonCallback(window->glfwWindow, input_mouse_button_callback);
  glfwSetCursorPosCallback(window->glfwWindow, input_mouse_position_callback);

  return OBJECT_VAL(mesche_object_make_pointer_type((VM *)mem, input_state,
                                                    &SubstInputStateType));
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
    return FALSE_VAL;
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

    return OBJECT_VAL(mesche_object_make_pointer_type((VM *)mem, input_event,
                                                      &SubstInputEventType));
  } else {
    return FALSE_VAL;
  }
}

Value input_event_key_code_msc(MescheMemory *mem, int arg_count, Value *args) {
  ObjectPointer *ptr = AS_POINTER(args[0]);
  SubstInputKeyEvent *input_event = (SubstInputKeyEvent *)ptr->ptr;
  return NUMBER_VAL(input_event->key_code);
}

Value input_event_key_down_msc(MescheMemory *mem, int arg_count, Value *args) {
  ObjectPointer *ptr = AS_POINTER(args[0]);
  SubstInputEvent *input_event = (SubstInputEvent *)(ptr->ptr);
  return BOOL_VAL(input_event->kind == INPUT_EVENT_KEY_DOWN);
}

Value input_event_key_up_msc(MescheMemory *mem, int arg_count, Value *args) {
  ObjectPointer *ptr = AS_POINTER(args[0]);
  SubstInputEvent *input_event = (SubstInputEvent *)ptr->ptr;
  return BOOL_VAL(input_event->kind == INPUT_EVENT_KEY_UP);
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

Value input_event_mouse_button_down_msc(MescheMemory *mem, int arg_count,
                                        Value *args) {
  ObjectPointer *ptr = AS_POINTER(args[0]);
  SubstInputEvent *input_event = (SubstInputEvent *)ptr->ptr;
  return BOOL_VAL(input_event->kind == INPUT_EVENT_MOUSE_BUTTON_DOWN);
}

Value input_event_mouse_button_up_msc(MescheMemory *mem, int arg_count,
                                      Value *args) {
  ObjectPointer *ptr = AS_POINTER(args[0]);
  SubstInputEvent *input_event = (SubstInputEvent *)ptr->ptr;
  return BOOL_VAL(input_event->kind == INPUT_EVENT_MOUSE_BUTTON_UP);
}

Value input_event_mouse_button_msc(MescheMemory *mem, int arg_count,
                                   Value *args) {
  ObjectPointer *ptr = AS_POINTER(args[0]);
  SubstInputMouseButtonEvent *input_event =
      (SubstInputMouseButtonEvent *)ptr->ptr;
  return NUMBER_VAL(input_event->button);
}

Value input_event_mouse_scroll_msc(MescheMemory *mem, int arg_count,
                                   Value *args) {
  ObjectPointer *ptr = AS_POINTER(args[0]);
  SubstInputEvent *input_event = (SubstInputEvent *)ptr->ptr;
  return BOOL_VAL(input_event->kind == INPUT_EVENT_MOUSE_SCROLL);
}

Value input_event_mouse_scroll_x_msc(MescheMemory *mem, int arg_count,
                                     Value *args) {
  ObjectPointer *ptr = AS_POINTER(args[0]);
  SubstInputMouseScrollEvent *input_event =
      (SubstInputMouseScrollEvent *)ptr->ptr;
  return NUMBER_VAL(input_event->offset_x);
}

Value input_event_mouse_scroll_y_msc(MescheMemory *mem, int arg_count,
                                     Value *args) {
  ObjectPointer *ptr = AS_POINTER(args[0]);
  SubstInputMouseScrollEvent *input_event =
      (SubstInputMouseScrollEvent *)ptr->ptr;
  return NUMBER_VAL(input_event->offset_y);
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
          {"input-event-mouse-button-down?", input_event_mouse_button_down_msc,
           true},
          {"input-event-mouse-button-up?", input_event_mouse_button_up_msc,
           true},
          {"input-event-mouse-button", input_event_mouse_button_msc, true},
          {"input-event-mouse-scroll?", input_event_mouse_scroll_msc, true},
          {"input-event-mouse-scroll-x", input_event_mouse_scroll_x_msc, true},
          {"input-event-mouse-scroll-y", input_event_mouse_scroll_y_msc, true},
          {NULL, NULL, false}});
}
