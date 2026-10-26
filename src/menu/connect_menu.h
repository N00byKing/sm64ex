#ifndef CONNECT_MENU_H
#define CONNECT_MENU_H

#include <PR/ultratypes.h>

#include "types.h"

// Archipelago connections on the file select screen: the CONNECT button in file_select.c grows into a
// menu that first shows the four files, then a form (server, port, name, password) to create or edit
// the connection saved in the chosen file.
//
// The buttons are real menu button objects that file_select.c spawns, laid out like the score menu.
// This file only draws the text and reads the input.

// Where the buttons of the menu are, as offsets from the button that spawns them (the same as the score menu).
#define CONNECT_BUTTON_FILE_X(i) (((i) % 2) == 0 ? 711 : -166)
#define CONNECT_BUTTON_FILE_Y(i) ((i) < 2 ? 311 : 0)
#define CONNECT_BUTTON_RETURN_X 711
#define CONNECT_BUTTON_RETURN_Y -388
#define CONNECT_BUTTON_SAVE_X -711
#define CONNECT_BUTTON_SAVE_Y -388

enum ConnectPage {
    CONNECT_PAGE_FILES,
    CONNECT_PAGE_FORM,
};

s32 connect_menu_is_open(void);
void connect_menu_open(void);
void connect_menu_close(void);

// The page that is being shown, file_select.c keeps its button objects in step with it.
s32 connect_menu_get_page(void);

// Briefly tells the player that the file they picked has no connection saved to it.
void connect_menu_show_no_connection_notice(void);

// Call once per frame with the last cursor click position (in cursor space, -10000 if there was none).
void connect_menu_update(s16 clickX, s16 clickY);

// Draws the current page if the menu is open, otherwise the notice if there is one.
// The cursor position is in cursor space, alpha is the opacity of the text.
void connect_menu_draw(f32 cursorX, f32 cursorY, u8 alpha);

#endif // CONNECT_MENU_H
