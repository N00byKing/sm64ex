#include <PR/ultratypes.h>
#include <PR/gbi.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio/external.h"
#include "connect_menu.h"
#include "game/game_init.h"
#include "game/ingame_menu.h"
#include "game/save_file.h"
#include "game/segment2.h"
#include "game/segment7.h"
#include "pc/text_input.h"
#include "sm64.h"

#if !defined(VERSION_JP) && !defined(VERSION_SH)
s16 get_string_width(u8 *str);
#endif
void create_dl_scale_matrix(s8 pushOp, f32 x, f32 y, f32 z);

/**
 * @file connect_menu.c
 * The text and input of the connections menu, which is laid out like the score menu: a title, the four
 * files in a grid and a row of buttons at the bottom. The buttons themselves are objects spawned by
 * file_select.c, so the rectangles here only have to cover them for the cursor.
 * All rectangles are in screen space (320x240, y pointing down), except for text baselines
 * which, like print_generic_string, are measured from the bottom of the screen.
 *
 * It is operated either with the keyboard (typing, Tab, Enter, Esc) or with the file select cursor.
 */

#define CLICK_NONE (-10000)

// The menu font is drawn at this fraction of its size so more fits in the fields.
#define TEXT_SCALE 0.8f
// Height of a glyph cell, which sits on top of the baseline.
#define TEXT_HEIGHT ((s16) (16 * TEXT_SCALE))

#define DEFAULT_SERVER "archipelago.gg"

enum Field {
    FIELD_SERVER,
    FIELD_PORT,
    FIELD_NAME,
    FIELD_PASSWORD,
    NUM_FIELDS,
};

// Focusable elements of the form: the fields followed by two buttons.
enum FormFocus {
    FOCUS_SAVE = NUM_FIELDS,
    FOCUS_RETURN,
    NUM_FORM_FOCUS,
};

// Focusable elements of the file page: the four files followed by a button.
enum FilesFocus {
    FILES_FOCUS_RETURN = NUM_SAVE_FILES,
    NUM_FILES_FOCUS,
};

struct Rect {
    s16 x1, y1, x2, y2;
};

#define PORT_MAX_LEN 5
static const u8 sFieldMaxLen[NUM_FIELDS] = {
    AP_SERVER_LEN - 1 - (PORT_MAX_LEN + 1), // leaves room for ":port"
    PORT_MAX_LEN,
    AP_NAME_LEN - 1,
    AP_PASSWORD_LEN - 1,
};

static const char *sFieldLabels[NUM_FIELDS] = { "SERVER", "PORT", "NAME", "PASSWORD" };

#define FIELD_TOP 56
#define FIELD_PITCH 22
#define FIELD_HEIGHT 18
#define FIELD_X1 100
#define FIELD_X2 280
#define FIELD_LABEL_X 40
#define FIELD_TEXT_PAD 6

// The names of the files use the score menu's layout (menu font, y measured from the top).
static const s16 sFileNameX[2] = { 89, 211 };
static const s16 sFileNameY[2] = { 62, 105 };
#define FILE_NAME_WIDTH 62 // the space next to a file button

// The buttons are objects placed by file_select.c, so where they are on screen is worked out the same way
// check_clicked_button does it for the other menus: their offset from the button they were spawned by,
// projected onto the screen, with a size of 50 by 42 pixels.
#define OFFSET_TO_SCREEN(v) ((v) * 13873 / 100000)

static struct Rect button_rect(s16 offsetX, s16 offsetY) {
    const s16 centerX = 160 - OFFSET_TO_SCREEN(offsetX);
    const s16 centerY = 120 - OFFSET_TO_SCREEN(offsetY);
    struct Rect r = { centerX - 25, centerY - 21, centerX + 25, centerY + 21 };
    return r;
}

static struct Rect file_button_rect(s32 i) {
    return button_rect(CONNECT_BUTTON_FILE_X(i), CONNECT_BUTTON_FILE_Y(i));
}

static struct Rect return_button_rect(void) {
    return button_rect(CONNECT_BUTTON_RETURN_X, CONNECT_BUTTON_RETURN_Y);
}

static struct Rect save_button_rect(void) {
    return button_rect(CONNECT_BUTTON_SAVE_X, CONNECT_BUTTON_SAVE_Y);
}

// What can be clicked: a file includes its text next to the button, the bottom buttons include their labels.
static struct Rect file_area(s32 i) {
    struct Rect r = file_button_rect(i);
    r.x2 += FILE_NAME_WIDTH + 8;
    return r;
}

static struct Rect bottom_button_area(struct Rect r) {
    r.y2 = 208;
    return r;
}

#define BUTTON_LABEL_BASELINE 35

// The status message goes in the gap between the buttons of the bottom row.
#define MESSAGE_BASELINE 57

#define NOTICE_FRAMES 90

static const struct Rect sNoticeBox = { 40, 92, 280, 148 };

static s32 sOpen = FALSE;
static s32 sNoticeTimer = 0;
static enum ConnectPage sPage = CONNECT_PAGE_FILES;
static s32 sFocus = 0;
static s32 sEditFile = 0; // the file the form is editing
static char sFields[NUM_FIELDS][AP_SERVER_LEN];
static s32 sFieldTooLong[NUM_FIELDS]; // something typed or pasted didn't fit and was left out
static char sMessage[40] = "";
static s32 sMessageIsError = FALSE;
static u8 sAlpha = 255;
// The focus is only shown once the keyboard is used, the cursor doesn't need it
static s32 sFocusVisible = FALSE;

/* helpers */

static struct Rect field_rect(s32 field) {
    struct Rect r = { FIELD_X1, FIELD_TOP + field * FIELD_PITCH, FIELD_X2, FIELD_TOP + field * FIELD_PITCH + FIELD_HEIGHT };
    return r;
}

// Baseline of the text in a field, which keeps it in the middle of the box.
static s16 field_baseline(s32 field) {
    return 224 - (FIELD_TOP + field * FIELD_PITCH);
}

static s32 rect_contains(const struct Rect *r, s16 x, s16 y) {
    return x >= r->x1 && x < r->x2 && y >= r->y1 && y < r->y2;
}

// Cursor space (origin in the middle, y up) to screen space (origin top left, y down).
static s16 cursor_to_screen_x(f32 x) {
    return (s16) (x + 160.0f);
}

static s16 cursor_to_screen_y(f32 y) {
    return (s16) (120.0f - y);
}

static void set_message(const char *msg, s32 isError) {
    snprintf(sMessage, sizeof(sMessage), "%s", msg);
    sMessageIsError = isError;
}

// Width in screen pixels, with TEXT_SCALE applied.
static s16 text_width(const u8 *str) {
#if !defined(VERSION_JP) && !defined(VERSION_SH)
    return (s16) (get_string_width((u8 *) str) * TEXT_SCALE);
#else
    s16 width = 0;
    while (*str++ != DIALOG_CHAR_TERMINATOR) width += 10;
    return (s16) (width * TEXT_SCALE);
#endif
}

static s32 dialog_length(const u8 *str) {
    s32 len = 0;
    while (str[len] != DIALOG_CHAR_TERMINATOR) len++;
    return len;
}

// The menu font only has letters, digits and a bit of punctuation. Everything else shows up as '?'
// (the stored text is still exactly what was typed or pasted).
static u8 ascii_to_dialog(char c) {
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
        return ASCII_TO_DIALOG(c);
    }
    switch (c) {
        case ' ':  return DIALOG_CHAR_SPACE;
        case '\'': return 0x3E;
        case '.':  return 0x3F;
        case ',':  return DIALOG_CHAR_COMMA;
        case '-':  return 0x9F;
        case '(':  return 0xE1;
        case ')':  return 0xE3;
        case '+':  return 0xE4;
        case '&':  return 0xE5;
        case ':':  return 0xE6;
        case '!':  return 0xF2;
        case '%':  return 0xF3;
        case '?':  return 0xF4;
        case '~':  return 0xF7;
        case '$':  return 0xF9;
        default:   return 0xF4;
    }
}

// Converts UTF-8 text. A character that isn't ASCII becomes a single '?', however many bytes it takes.
static void to_dialog(const char *src, u8 *dst, size_t dstSize) {
    const unsigned char *s = (const unsigned char *) src;
    size_t i = 0;

    while (*s != '\0' && i < dstSize - 1) {
        if (*s < 0x80) {
            dst[i++] = ascii_to_dialog(*s++);
        } else {
            dst[i++] = ascii_to_dialog('?');
            s++;
            while ((*s & 0xC0) == 0x80) s++; // the rest of the bytes of this character
        }
    }
    dst[i] = DIALOG_CHAR_TERMINATOR;
}

/* drawing */

static void draw_rect(s16 x1, s16 y1, s16 x2, s16 y2, u8 r, u8 g, u8 b) {
    gDPPipeSync(gDisplayListHead++);
    gDPSetRenderMode(gDisplayListHead++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetCycleType(gDisplayListHead++, G_CYC_FILL);
    gDPSetFillColor(gDisplayListHead++, GPACK_RGBA5551(r, g, b, 255));
    gDPFillRectangle(gDisplayListHead++, x1, y1, x2 - 1, y2 - 1);
    gDPPipeSync(gDisplayListHead++);
    gDPSetCycleType(gDisplayListHead++, G_CYC_1CYCLE);
}

// A rectangle with a 2 pixel border.
static void draw_box(const struct Rect *r, u8 bgR, u8 bgG, u8 bgB, u8 borderR, u8 borderG, u8 borderB) {
    draw_rect(r->x1 - 2, r->y1 - 2, r->x2 + 2, r->y2 + 2, borderR, borderG, borderB);
    draw_rect(r->x1, r->y1, r->x2, r->y2, bgR, bgG, bgB);
}

// Only the border of a rectangle, to show what has the keyboard focus.
static void draw_outline(const struct Rect *r) {
    draw_rect(r->x1, r->y1, r->x2, r->y1 + 2, 255, 255, 255);
    draw_rect(r->x1, r->y2 - 2, r->x2, r->y2, 255, 255, 255);
    draw_rect(r->x1, r->y1, r->x1 + 2, r->y2, 255, 255, 255);
    draw_rect(r->x2 - 2, r->y1, r->x2, r->y2, 255, 255, 255);
}

static void print_scaled(s16 x, s16 y, const u8 *str) {
    create_dl_translation_matrix(MENU_MTX_PUSH, x, y, 0.0f);
    create_dl_scale_matrix(MENU_MTX_NOPUSH, TEXT_SCALE, TEXT_SCALE, 1.0f);
    print_generic_string(0, 0, str);
    gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
}

static void draw_text(s16 x, s16 y, const u8 *str, u8 r, u8 g, u8 b) {
    gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
    gDPSetEnvColor(gDisplayListHead++, r, g, b, sAlpha);
    print_scaled(x, y, str);
    gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
}

static void draw_text_centered(s16 centerX, s16 y, const char *str, u8 r, u8 g, u8 b) {
    u8 buf[AP_SERVER_LEN + 1];
    to_dialog(str, buf, sizeof(buf));
    draw_text(centerX - text_width(buf) / 2, y, buf, r, g, b);
}

// Draws the end of a string if it doesn't fit in maxWidth, returns the width that was drawn.
static s16 draw_text_tail(s16 x, s16 y, const char *str, s16 maxWidth, s32 mask) {
    u8 buf[AP_SERVER_LEN + 1];
    s32 start = 0;

    to_dialog(str, buf, sizeof(buf));
    if (mask) {
        for (s32 i = 0; buf[i] != DIALOG_CHAR_TERMINATOR; i++) buf[i] = 0x3F; // '.'
    }
    while (buf[start] != DIALOG_CHAR_TERMINATOR && text_width(buf + start) > maxWidth) start++;

    draw_text(x, y, buf + start, 255, 255, 255);
    return text_width(buf + start);
}

// Draws the start of a string, cut off if it doesn't fit in maxWidth.
static void draw_text_head(s16 x, s16 y, const char *str, s16 maxWidth, u8 r, u8 g, u8 b) {
    u8 buf[AP_SERVER_LEN + 1];
    s32 len;

    to_dialog(str, buf, sizeof(buf));
    len = dialog_length(buf);
    while (len > 0 && text_width(buf) > maxWidth) buf[--len] = DIALOG_CHAR_TERMINATOR;
    draw_text(x, y, buf, r, g, b);
}

// The title in the big HUD font, at the same place as the other menus have theirs.
static void draw_title(const char *title) {
    u8 buf[32];
    to_dialog(title, buf, sizeof(buf));

    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_begin);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, sAlpha);
    print_hud_lut_string(HUD_LUT_GLOBAL, 160 - (s16) strlen(title) * 6, 35, buf); // the HUD font is 12 wide
    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_end);
}

static void draw_message(void) {
    if (sMessage[0] == '\0') return;

    if (sMessageIsError) {
        draw_text_centered(160, MESSAGE_BASELINE, sMessage, 255, 150, 150);
    } else {
        draw_text_centered(160, MESSAGE_BASELINE, sMessage, 190, 255, 190);
    }
}

// The labels under the buttons of the bottom row.
static void draw_button_labels(s32 showSave) {
    const struct Rect returnButton = return_button_rect();
    const struct Rect saveButton = save_button_rect();

    draw_text_centered((returnButton.x1 + returnButton.x2) / 2, BUTTON_LABEL_BASELINE, "RETURN", 255, 255, 255);
    if (showSave) draw_text_centered((saveButton.x1 + saveButton.x2) / 2, BUTTON_LABEL_BASELINE, "SAVE", 255, 255, 255);
}

static void draw_form(void) {
    char title[8];
    snprintf(title, sizeof(title), "MARIO %c", 'A' + sEditFile);
    draw_title(title);

    for (s32 i = 0; i < NUM_FIELDS; i++) {
        const struct Rect r = field_rect(i);
        const s32 focused = (sFocus == i);
        const s16 baseline = field_baseline(i);
        u8 label[16];

        to_dialog(sFieldLabels[i], label, sizeof(label));
        draw_text(FIELD_LABEL_X, baseline, label, 255, 255, 255);

        draw_box(&r, 0x10, 0x18, 0x28, focused ? 255 : 0x70, focused ? 255 : 0x80, focused ? 255 : 0x90);

        const s16 textWidth = draw_text_tail(r.x1 + FIELD_TEXT_PAD, baseline, sFields[i],
                                             r.x2 - r.x1 - FIELD_TEXT_PAD * 2 - 4, i == FIELD_PASSWORD);
        if (focused && (gGlobalTimer & 16)) {
            // blinking caret, the font has no underscore
            const s16 caretX = r.x1 + FIELD_TEXT_PAD + textWidth + 1;
            draw_rect(caretX, 240 - baseline - TEXT_HEIGHT, caretX + 2, 240 - baseline, 255, 255, 255);
        }
    }

    if (sFocus == FOCUS_SAVE) {
        const struct Rect r = save_button_rect();
        draw_outline(&r);
    }
    if (sFocus == FOCUS_RETURN) {
        const struct Rect r = return_button_rect();
        draw_outline(&r);
    }
    draw_button_labels(TRUE);
    draw_message();
}

static void draw_file_names(void) {
    // The names use the menu font, like the score menu does
    gSPDisplayList(gDisplayListHead++, dl_menu_ia8_text_begin);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, sAlpha);
    for (s32 i = 0; i < NUM_SAVE_FILES; i++) {
        u8 name[8];
        char text[8];

        snprintf(text, sizeof(text), "MARIO %c", 'A' + i);
        to_dialog(text, name, sizeof(name));
        print_menu_generic_string(sFileNameX[i % 2], sFileNameY[i / 2], name);
    }
    gSPDisplayList(gDisplayListHead++, dl_menu_ia8_text_end);
}

static void draw_files(void) {
    draw_title("CONNECTIONS");
    draw_file_names();

    // What is saved in each file, where the score menu has the star count
    for (s32 i = 0; i < NUM_SAVE_FILES; i++) {
        const char *name = save_file_get_ap_name(i);
        const s16 x = sFileNameX[i % 2] + 2;
        const s16 baseline = 240 - (sFileNameY[i / 2] + 14) - TEXT_HEIGHT;

        if (name[0] != '\0') {
            draw_text_head(x, baseline, name, FILE_NAME_WIDTH, 190, 255, 190);
        } else {
            draw_text_head(x, baseline, "NONE", FILE_NAME_WIDTH, 220, 220, 220);
        }
    }

    if (sFocusVisible) {
        const struct Rect r = (sFocus < NUM_SAVE_FILES) ? file_button_rect(sFocus) : return_button_rect();
        draw_outline(&r);
    }
    draw_button_labels(FALSE);
    draw_message();
}

/* input */

// Fills the form with the connection saved in a file, or with the defaults if it has none.
static void load_fields(s32 fileIndex) {
    const char *server = save_file_get_ap_server(fileIndex);
    const char *colon = strrchr(server, ':');
    const char *port = colon ? colon + 1 : "";
    s32 portIsNumber = (port[0] != '\0');

    for (const char *c = port; *c != '\0'; c++) {
        if (*c < '0' || *c > '9') portIsNumber = FALSE;
    }

    memset(sFields, 0, sizeof(sFields));
    memset(sFieldTooLong, 0, sizeof(sFieldTooLong));
    if (server[0] == '\0') {
        snprintf(sFields[FIELD_SERVER], sizeof(sFields[FIELD_SERVER]), "%s", DEFAULT_SERVER);
        return;
    }

    if (portIsNumber && strlen(port) <= PORT_MAX_LEN) {
        snprintf(sFields[FIELD_SERVER], sizeof(sFields[FIELD_SERVER]), "%.*s", (int) (colon - server), server);
        snprintf(sFields[FIELD_PORT], sizeof(sFields[FIELD_PORT]), "%s", port);
    } else {
        snprintf(sFields[FIELD_SERVER], sizeof(sFields[FIELD_SERVER]), "%s", server);
    }
    snprintf(sFields[FIELD_NAME], sizeof(sFields[FIELD_NAME]), "%s", save_file_get_ap_name(fileIndex));
    snprintf(sFields[FIELD_PASSWORD], sizeof(sFields[FIELD_PASSWORD]), "%s", save_file_get_ap_password(fileIndex));
}

static void close_menu(void) {
    sOpen = FALSE;
    text_input_set_active(FALSE);
    play_sound(SOUND_MENU_CLICK_FILE_SELECT, gDefaultSoundArgs);
}

static void open_form(s32 fileIndex) {
    sEditFile = fileIndex;
    sPage = CONNECT_PAGE_FORM;
    sFocus = FIELD_SERVER;
    sMessage[0] = '\0';
    load_fields(fileIndex);
    play_sound(SOUND_MENU_CLICK_FILE_SELECT, gDefaultSoundArgs);
}

// Back to the files, with the file that was just edited focused.
static void back_to_files(void) {
    sPage = CONNECT_PAGE_FILES;
    sFocus = sEditFile;
    sMessage[0] = '\0';
    play_sound(SOUND_MENU_CLICK_FILE_SELECT, gDefaultSoundArgs);
}

// Writes a code point as UTF-8 and returns how many bytes it took (1 to 4).
static s32 encode_utf8(u32 codepoint, char *out) {
    if (codepoint < 0x80) {
        out[0] = codepoint;
        return 1;
    } else if (codepoint < 0x800) {
        out[0] = 0xC0 | (codepoint >> 6);
        out[1] = 0x80 | (codepoint & 0x3F);
        return 2;
    } else if (codepoint < 0x10000) {
        out[0] = 0xE0 | (codepoint >> 12);
        out[1] = 0x80 | ((codepoint >> 6) & 0x3F);
        out[2] = 0x80 | (codepoint & 0x3F);
        return 3;
    }
    out[0] = 0xF0 | (codepoint >> 18);
    out[1] = 0x80 | ((codepoint >> 12) & 0x3F);
    out[2] = 0x80 | ((codepoint >> 6) & 0x3F);
    out[3] = 0x80 | (codepoint & 0x3F);
    return 4;
}

// The fields hold UTF-8 and are limited in bytes. A character that doesn't fit is left out whole, never cut in half.
static void append_codepoint(s32 field, u32 codepoint) {
    char bytes[4];
    const size_t len = strlen(sFields[field]);
    s32 count;

    if (field == FIELD_PORT && (codepoint < '0' || codepoint > '9')) return;

    count = encode_utf8(codepoint, bytes);
    if (len + count > sFieldMaxLen[field]) {
        if (field != FIELD_PORT) sFieldTooLong[field] = TRUE;
        return;
    }

    memcpy(sFields[field] + len, bytes, count);
    sFields[field][len + count] = '\0';
    sMessage[0] = '\0';
}

// Removes the last character, which can be several bytes.
static void delete_char(s32 field) {
    size_t len = strlen(sFields[field]);

    if (len > 0) {
        do {
            len--;
        } while (len > 0 && (sFields[field][len] & 0xC0) == 0x80); // step back over the continuation bytes
        sFields[field][len] = '\0';
    }
    sFieldTooLong[field] = FALSE;
    sMessage[0] = '\0';
}

static void show_error(const char *msg, s32 focus) {
    set_message(msg, TRUE);
    sFocus = focus;
    play_sound(SOUND_MENU_CAMERA_BUZZ, gDefaultSoundArgs);
}

static void save_to_file(s32 fileIndex) {
    char server[AP_SERVER_LEN];

    if (sFields[FIELD_PORT][0] != '\0') {
        snprintf(server, sizeof(server), "%s:%s", sFields[FIELD_SERVER], sFields[FIELD_PORT]);
    } else {
        snprintf(server, sizeof(server), "%s", sFields[FIELD_SERVER]);
    }
    save_file_set_ap_connection(fileIndex, server, sFields[FIELD_NAME], sFields[FIELD_PASSWORD]);

    snprintf(sMessage, sizeof(sMessage), "SAVED MARIO %c", 'A' + fileIndex);
    sMessageIsError = FALSE;
    sPage = CONNECT_PAGE_FILES;
    sFocus = fileIndex;
    play_sound(SOUND_MENU_STAR_SOUND, gDefaultSoundArgs);
}

// Checks the form and saves it to the file being edited.
static void try_save(void) {
    const s32 port = atoi(sFields[FIELD_PORT]);

    if (sFields[FIELD_SERVER][0] == '\0') {
        show_error("ENTER A SERVER", FIELD_SERVER);
    } else if (sFields[FIELD_PORT][0] != '\0' && (port < 1 || port > 65535)) {
        show_error("PORT MUST BE 1 TO 65535", FIELD_PORT);
    } else if (sFields[FIELD_NAME][0] == '\0') {
        show_error("ENTER A NAME", FIELD_NAME);
    } else if (sFieldTooLong[FIELD_SERVER]) {
        show_error("SERVER TOO LONG", FIELD_SERVER);
    } else if (sFieldTooLong[FIELD_NAME]) {
        show_error("NAME TOO LONG", FIELD_NAME);
    } else if (sFieldTooLong[FIELD_PASSWORD]) {
        show_error("PASSWORD TOO LONG", FIELD_PASSWORD);
    } else {
        save_to_file(sEditFile);
    }
}

static void activate_files_focus(s32 focus) {
    if (focus == FILES_FOCUS_RETURN) {
        close_menu();
    } else {
        open_form(focus);
    }
}

static void move_focus(s32 delta) {
    const s32 count = (sPage == CONNECT_PAGE_FORM) ? NUM_FORM_FOCUS : NUM_FILES_FOCUS;

    sFocus = (sFocus + delta + count) % count;
    play_sound(SOUND_MENU_CHANGE_SELECT, gDefaultSoundArgs);
}

static void handle_key(const struct TextInputEvent *ev) {
    // On the files page the focus isn't shown until the keyboard is first used, and that first key only shows it
    const s32 focusWasVisible = sFocusVisible || (sPage == CONNECT_PAGE_FORM);
    sFocusVisible = TRUE;

    switch (ev->type) {
        case TEXT_INPUT_CHAR:
            if (sPage == CONNECT_PAGE_FORM) {
                if (sFocus < NUM_FIELDS) append_codepoint(sFocus, ev->codepoint);
            } else if (ev->codepoint >= 'a' && ev->codepoint <= 'd') {
                open_form(ev->codepoint - 'a');
            } else if (ev->codepoint >= 'A' && ev->codepoint <= 'D') {
                open_form(ev->codepoint - 'A');
            } else if (ev->codepoint >= '1' && ev->codepoint <= '4') {
                open_form(ev->codepoint - '1');
            }
            break;

        case TEXT_INPUT_BACKSPACE:
            if (sPage == CONNECT_PAGE_FORM && sFocus < NUM_FIELDS) delete_char(sFocus);
            break;

        case TEXT_INPUT_TAB:
        case TEXT_INPUT_DOWN:
        case TEXT_INPUT_RIGHT:
            if (focusWasVisible) move_focus(1);
            break;

        case TEXT_INPUT_SHIFT_TAB:
        case TEXT_INPUT_UP:
        case TEXT_INPUT_LEFT:
            if (focusWasVisible) move_focus(-1);
            break;

        case TEXT_INPUT_ENTER:
            if (sPage == CONNECT_PAGE_FILES) {
                if (focusWasVisible) activate_files_focus(sFocus);
            } else if (sFocus == FOCUS_RETURN) {
                back_to_files();
            } else if (sFocus == FIELD_PASSWORD || sFocus == FOCUS_SAVE) {
                try_save();
            } else {
                move_focus(1);
            }
            break;

        case TEXT_INPUT_ESCAPE:
            if (sPage == CONNECT_PAGE_FORM) {
                back_to_files();
            } else {
                close_menu();
            }
            break;
    }
}

static void handle_click(s16 x, s16 y) {
    const struct Rect returnArea = bottom_button_area(return_button_rect());
    const struct Rect saveArea = bottom_button_area(save_button_rect());

    sFocusVisible = FALSE;

    if (sPage == CONNECT_PAGE_FORM) {
        for (s32 i = 0; i < NUM_FIELDS; i++) {
            const struct Rect r = field_rect(i);
            if (rect_contains(&r, x, y)) {
                sFocus = i;
                play_sound(SOUND_MENU_CHANGE_SELECT, gDefaultSoundArgs);
                return;
            }
        }
        if (rect_contains(&saveArea, x, y)) {
            try_save();
        } else if (rect_contains(&returnArea, x, y)) {
            back_to_files();
        }
    } else {
        for (s32 i = 0; i < NUM_SAVE_FILES; i++) {
            const struct Rect fileArea = file_area(i);
            if (rect_contains(&fileArea, x, y)) {
                open_form(i);
                return;
            }
        }
        if (rect_contains(&returnArea, x, y)) {
            close_menu();
        }
    }
}

/* interface */

s32 connect_menu_is_open(void) {
    return sOpen;
}

void connect_menu_open(void) {
    sOpen = TRUE;
    sPage = CONNECT_PAGE_FILES;
    sFocus = 0;
    sFocusVisible = FALSE;
    sMessage[0] = '\0';
    text_input_set_active(TRUE);
}

void connect_menu_close(void) {
    sOpen = FALSE;
    sNoticeTimer = 0;
    text_input_set_active(FALSE);
}

s32 connect_menu_get_page(void) {
    return sPage;
}

void connect_menu_show_no_connection_notice(void) {
    sNoticeTimer = NOTICE_FRAMES;
}

void connect_menu_update(s16 clickX, s16 clickY) {
    const s32 clicked = (clickX != CLICK_NONE);
    const s16 x = cursor_to_screen_x(clickX);
    const s16 y = cursor_to_screen_y(clickY);

    if (!sOpen) return;

    struct TextInputEvent ev;
    while (sOpen && text_input_pop(&ev)) {
        handle_key(&ev);
    }
    if (sOpen && clicked) handle_click(x, y);
}

void connect_menu_draw(UNUSED f32 cursorX, UNUSED f32 cursorY, u8 alpha) {
    sAlpha = alpha;

    if (!sOpen) {
        if (sNoticeTimer > 0) {
            sNoticeTimer--;
            sAlpha = 255;
            draw_box(&sNoticeBox, 0x30, 0x30, 0x60, 255, 255, 255);
            draw_text_centered(160, 116, "NO CONNECTION SAVED", 255, 150, 150);
            draw_text_centered(160, 98, "USE CONNECT FIRST", 255, 255, 255);
        }
        return;
    }

    if (sPage == CONNECT_PAGE_FILES) {
        draw_files();
    } else {
        draw_form();
    }
}
