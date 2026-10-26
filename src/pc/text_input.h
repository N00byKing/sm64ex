#ifndef TEXT_INPUT_H
#define TEXT_INPUT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum TextInputEventType {
    TEXT_INPUT_CHAR,
    TEXT_INPUT_BACKSPACE,
    TEXT_INPUT_ENTER,
    TEXT_INPUT_TAB,
    TEXT_INPUT_SHIFT_TAB,
    TEXT_INPUT_ESCAPE,
    TEXT_INPUT_UP,
    TEXT_INPUT_DOWN,
    TEXT_INPUT_LEFT,
    TEXT_INPUT_RIGHT,
};

struct TextInputEvent {
    enum TextInputEventType type;
    unsigned int codepoint; // Unicode code point, only set for TEXT_INPUT_CHAR
};

// While text input is active the window backend routes typed characters and editing keys
// into a queue instead of to the controller bindings, so typing can't move Mario or the menu cursor.
void text_input_set_active(bool active);
bool text_input_is_active(void);

// Called by the window backend. Ignored while text input is inactive.
void text_input_push(enum TextInputEventType type, unsigned int codepoint);

// Pushes every character of a UTF-8 string (typed text or the clipboard) as a TEXT_INPUT_CHAR.
// Control characters (newlines, tabs, ...) and invalid UTF-8 are skipped.
void text_input_push_utf8(const char *utf8);

// Returns false once the queue is empty.
bool text_input_pop(struct TextInputEvent *ev);

#ifdef __cplusplus
}
#endif

#endif // TEXT_INPUT_H
