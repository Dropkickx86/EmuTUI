#ifndef TYPES_H_
#define TYPES_H_

#include <ncurses.h>

typedef struct _list_struct {
    int num_entries, size_content;
    char **content;
} LIST;

typedef struct _WIN_struct {
    int w, h;
    LIST content;
    char **action, **gamedir, *deffile;
    WINDOW *win;
} WIN;

typedef struct _MENU_struct {
    int num_win, focus_system, focus_entry, scrmaxy, scrmaxx;
    WIN *windows;
} MENU;

typedef struct _axis_state_struct {
    short x, y;
} AXIS_STATE;

#endif