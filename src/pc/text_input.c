#include "text_input.h"

#include "controller/controller_keyboard.h"

#ifdef WAPI_SDL2
#include <SDL2/SDL.h>
#endif

// Enough for a whole paste into any of the menu's fields
#define QUEUE_SIZE 256

#define INVALID_CODEPOINT 0xFFFFFFFFu

static struct TextInputEvent sQueue[QUEUE_SIZE];
static int sHead;
static int sCount;
static bool sActive;

void text_input_set_active(bool active) {
    if (active == sActive) return;
    sActive = active;
    sHead = 0;
    sCount = 0;

    if (active) {
        // don't carry keys that are already held (e.g. the one that opened the menu) into the text field
        keyboard_on_all_keys_up();
    }
#ifdef WAPI_SDL2
    if (active) SDL_StartTextInput();
    else SDL_StopTextInput();
#endif
}

bool text_input_is_active(void) {
    return sActive;
}

void text_input_push(enum TextInputEventType type, unsigned int codepoint) {
    if (!sActive || sCount == QUEUE_SIZE) return;
    // no control characters (newlines and tabs in a paste, DEL, and the C1 range)
    if (type == TEXT_INPUT_CHAR && (codepoint < 0x20 || (codepoint >= 0x7F && codepoint < 0xA0))) return;

    sQueue[(sHead + sCount) % QUEUE_SIZE] = (struct TextInputEvent) { type, codepoint };
    sCount++;
}

// Reads one character from a UTF-8 string and moves past it. Returns INVALID_CODEPOINT for a bad sequence
// (stray or missing continuation bytes, overlong encodings, surrogates, values above U+10FFFF), after
// skipping the bytes that were part of it.
static unsigned int decode_utf8(const unsigned char **str) {
    static const unsigned int minimum[5] = { 0, 0, 0x80, 0x800, 0x10000 };
    const unsigned char *s = *str;
    unsigned int codepoint;
    int length;
    int i;

    if (s[0] < 0x80) {
        codepoint = s[0];
        length = 1;
    } else if ((s[0] & 0xE0) == 0xC0) {
        codepoint = s[0] & 0x1F;
        length = 2;
    } else if ((s[0] & 0xF0) == 0xE0) {
        codepoint = s[0] & 0x0F;
        length = 3;
    } else if ((s[0] & 0xF8) == 0xF0) {
        codepoint = s[0] & 0x07;
        length = 4;
    } else {
        *str += 1;
        return INVALID_CODEPOINT;
    }

    for (i = 1; i < length; i++) {
        if ((s[i] & 0xC0) != 0x80) {
            *str += i;
            return INVALID_CODEPOINT;
        }
        codepoint = (codepoint << 6) | (s[i] & 0x3F);
    }
    *str += length;

    if (codepoint < minimum[length] || codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
        return INVALID_CODEPOINT;
    }
    return codepoint;
}

void text_input_push_utf8(const char *utf8) {
    const unsigned char *s = (const unsigned char *) utf8;

    while (*s != '\0') {
        const unsigned int codepoint = decode_utf8(&s);
        if (codepoint != INVALID_CODEPOINT) {
            text_input_push(TEXT_INPUT_CHAR, codepoint);
        }
    }
}

bool text_input_pop(struct TextInputEvent *ev) {
    if (sCount == 0) return false;

    *ev = sQueue[sHead];
    sHead = (sHead + 1) % QUEUE_SIZE;
    sCount--;
    return true;
}
