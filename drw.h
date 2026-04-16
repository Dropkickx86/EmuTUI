#include "types.h"

WIN win_create(int height, int starty, int startx, char *filepath);
void menu_shift(const MENU *menu);
void menu_scroll(const MENU *menu, bool highlight);
WINDOW** edit_window_create(const MENU *menu);
int entry_edit(MENU *menu);