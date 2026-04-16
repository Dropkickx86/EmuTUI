#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "filehandler.h"
#include "log.h"
#include "types.h"

//Function to create and populate a window struct
//Returns WIN.content.content == NULL on fail
WIN win_create(int height, int starty, int startx, char *filepath) {
    //Init local WIN
    WIN win_local = {0, 0, {0, 0, NULL}, NULL, NULL, strdup(filepath), NULL};

    //Input validation
    if (height < 1) {
        log_write(22, "Error: Invalid window dimentions");
        return win_local;
    }
    else if (starty < 0 || startx < 0) {
        log_write(22, "Error: Invalid window position");
        return win_local;
    }
    else if (filepath[0] != '/' || strlen(filepath) < 2) {
        log_write(22, "Error: Invalid filepath");
        return win_local;
    }
    win_local.h = height;
    
    //Allocate and fill content
    win_content_get(&win_local);
    if (!win_local.content.content) {
        log_write(errno, "Error: Could not get content from file %s", filepath);
        return win_local;
    }
   
    //Create window
    win_local.win = newwin(win_local.h, win_local.w, starty, startx);
    if (!win_local.win) {
        free(win_local.action);
        free(*win_local.content.content);
        free(win_local.content.content);
        win_local.action = NULL;
        win_local.content.content = NULL;
    }
    return win_local;
}

//Function to move right and left in the menu
void menu_shift(const MENU *menu) {
    //Input validation
    if (!menu->windows[menu->focus_system].win) {
        log_write(22, "Error: Window does not exist");
        return;
    }

    //Clear screen to avoid artefacts
    clear();
    refresh();
    
    //Draw currently focused window in the middle
    mvwin(menu->windows[menu->focus_system].win, 0, (menu->scrmaxx / 2) - (menu->windows[menu->focus_system].w / 2));
    wnoutrefresh(menu->windows[menu->focus_system].win);

    //Draw the windows on the right side of the screen
    int offset = ((menu->scrmaxx / 2) - (menu->windows[menu->focus_system].w / 2) + menu->windows[menu->focus_system].w);
    for (int i = menu->focus_system + 1; i < menu->num_win; i++) {
        if ((offset + menu->windows[i].w) > (menu->scrmaxx + 1)) {
            break;
        }
        mvwin(menu->windows[i].win, 0, offset);
        offset += menu->windows[i].w;
        wnoutrefresh(menu->windows[i].win);
    }

    //Draw the windows on the left side of the screen
    offset = ((menu->scrmaxx / 2) - (menu->windows[menu->focus_system].w / 2));
    for (int i = menu->focus_system - 1; i >= 0; i--) {
        offset -= menu->windows[i].w;
        if (offset < 0) {
            break;
        }
        mvwin(menu->windows[i].win, 0, offset);
        wnoutrefresh(menu->windows[i].win);
    }

    //Update screen
    doupdate();
}

//Function to move up and down in the menu
void menu_scroll(const MENU *menu, bool highliht) {
    //Input validation
    if (!menu->windows[menu->focus_system].win) {
        log_write(22, "Error: Window does not exist");
        return;
    }

    //Clear the window to avoid artefacts
    wclear(menu->windows[menu->focus_system].win);

    //Set position of the highest entry
    int position = (menu->windows[menu->focus_system].h / 2) - menu->focus_entry;
    int entry = 0;

    //If list extends past the top of the window, set which entry will be on top of the window
    if (position < 0) {
        position = 0;
        entry = menu->focus_entry - (menu->windows[menu->focus_system].h / 2);
    }

    //Draw until all entries are drawn or end of window is reached.
    while (entry <= menu->windows[menu->focus_system].h && entry < menu->windows[menu->focus_system].content.num_entries) {
        if (entry == menu->focus_entry && highliht) {
            wattr_on(menu->windows[menu->focus_system].win, COLOR_PAIR(1), NULL);
        }
        if (entry == 0 && menu->focus_system != 0) {
            mvwaddstr(menu->windows[menu->focus_system].win, position++, ((menu->windows[menu->focus_system].w / 2) - 2), " Scan ");
        }
        else {
            mvwaddch(menu->windows[menu->focus_system].win, position, ((menu->windows[menu->focus_system].w / 2) - (strlen(menu->windows[menu->focus_system].content.content[entry]) / 2) - 1), ' ');
            mvwaddstr(menu->windows[menu->focus_system].win, position, ((menu->windows[menu->focus_system].w / 2) - (strlen(menu->windows[menu->focus_system].content.content[entry]) / 2)), menu->windows[menu->focus_system].content.content[entry]);
            mvwaddch(menu->windows[menu->focus_system].win, position++, ((menu->windows[menu->focus_system].w / 2) - (strlen(menu->windows[menu->focus_system].content.content[entry]) / 2) + strlen(menu->windows[menu->focus_system].content.content[entry])), ' ');
        }
        if (entry == menu->focus_entry && highliht) {
            wattr_off(menu->windows[menu->focus_system].win, COLOR_PAIR(1), NULL);
        }
        entry++;
    }

    //Update window
    wrefresh(menu->windows[menu->focus_system].win);
}

//Function to draw a window for editing entries
//Returns NULL on fail
WINDOW** edit_window_create(const MENU *menu) {
    if (menu->scrmaxy < 7 || menu->scrmaxx < 60) {
        log_write(95, "Error: Screen too small to draw edit window");
        return NULL;
    }
    int y, x;
    y = (menu->scrmaxy / 2) - 3;
    x = (menu->scrmaxx / 2) - 30;

    //Setup windows making up the edit window
    WINDOW **local_win_array = (WINDOW**) malloc(5 * sizeof(WINDOW*));
    if (!local_win_array) {
        log_write(errno, "Error: Could not allocate memory");
        local_win_array = NULL;
        return local_win_array;
    }
    local_win_array[4] = newwin(7, 60, y, x);
    local_win_array[0] = derwin(local_win_array[4], 1, 51, 1, 7);
    local_win_array[1] = derwin(local_win_array[4], 1, 51, 3, 7);
    local_win_array[2] = derwin(local_win_array[4], 1, 4, 5, 6);
    local_win_array[3] = derwin(local_win_array[4], 1, 8, 5, 12);
    
    //Add static values
    box(local_win_array[4], 0, 0);
    mvwaddstr(local_win_array[4], 1, 2, "Name:");
    mvwaddstr(local_win_array[4], 3, 2, "File:");
    mvwaddstr(local_win_array[2], 0, 0, "<OK>");
    mvwaddstr(local_win_array[3], 0, 0, "<CANCEL>");
    
    wrefresh(local_win_array[4]);
    return local_win_array;
}

//Function for editing an entry
//Returns 0 on success
int entry_edit(MENU *menu) {
    //Input validation
    if (!menu->windows[menu->focus_system].content.content[menu->focus_entry]) {
        log_write(22, "Error: Content does not exist");
        return 22;
    }

    //Create and show the edit window
    WINDOW **edit_window = edit_window_create(menu);
    if (!edit_window) {
        log_write(-1, "Error: Failed to create edit window");
        return -1;
    }

    //Show cursor and activate numpad keys
    curs_set(1);
    for (int i = 0; i < 5; i++) {
        keypad(edit_window[i], true);
    }

    //Save current name and path in temp array
    int focus = 0, focus_content[2], focus_button = 0, key_pressed, retval = 0;
    char content_new[2][51];
    if (menu->focus_entry == 0) {
        strcpy(content_new[0], "Scan");
        strcpy(content_new[1], menu->windows[menu->focus_system].content.content[menu->focus_entry]);
    }
    else {
        strcpy(content_new[0], menu->windows[menu->focus_system].content.content[menu->focus_entry]);
        strcpy(content_new[1], menu->windows[menu->focus_system].action[menu->focus_entry]);
    }

    //Save cursor position to last char, write current values to edit window
    focus_content[0] = strlen(content_new[0]);
    focus_content[1] = strlen(content_new[1]);
    mvwaddstr(edit_window[0], 0, 0, content_new[0]);
    mvwaddstr(edit_window[1], 0, 0, content_new[1]);
    wrefresh(edit_window[1]);
    wmove(edit_window[0], 0, strlen(content_new[0]));
    wrefresh(edit_window[0]);
    
    //Main loop for editing
    while ((key_pressed = wgetch(edit_window[focus])) != KEY_F(1)) {
        switch (key_pressed) {
            case KEY_UP: {
                //Move focus between elements
                if (focus == 2) {
                    curs_set(1);
                    wbkgd(edit_window[focus + focus_button], ' ');
                    wrefresh(edit_window[focus + focus_button]);
                    focus_button = 0;
                }
                if (focus > 0) {
                    focus--;
                    wmove(edit_window[focus], 0, focus_content[focus]);
                    wrefresh(edit_window[focus]);
                }
                break;
            }
            case KEY_DOWN: {
                //Move focus between elements
                if (focus < 2) {
                    focus++;
                    if (focus < 2) {
                        wmove(edit_window[focus], 0, focus_content[focus]);
                        wrefresh(edit_window[focus]);
                    }
                    else {
                        curs_set(0);
                        wbkgd(edit_window[focus + focus_button], ' ' | COLOR_PAIR(1));
                        wrefresh(edit_window[focus + focus_button]);
                    }
                }
                break;
            }
            case KEY_LEFT: {
                //Move between <OK> and <CANCEL>
                if (focus == 2) {
                    wbkgd(edit_window[focus + focus_button], ' ');
                    wrefresh(edit_window[focus + focus_button]);
                    focus_button = !focus_button;
                    wbkgd(edit_window[focus + focus_button], ' ' | COLOR_PAIR(1));
                    wrefresh(edit_window[focus + focus_button]);
                }
                //Move cursor in word
                else {
                    if (focus_content[focus] > 0) {
                        focus_content[focus]--;
                        wmove(edit_window[focus], 0, focus_content[focus]);
                        wrefresh(edit_window[focus]);
                    }
                }
                break;
            }
            case KEY_RIGHT: {
                //Move between <OK> and <CANCEL>
                if (focus == 2) {
                    wbkgd(edit_window[focus + focus_button], ' ');
                    wrefresh(edit_window[focus + focus_button]);
                    focus_button = !focus_button;
                    wbkgd(edit_window[focus + focus_button], ' ' | COLOR_PAIR(1));
                    wrefresh(edit_window[focus + focus_button]);
                }
                //Move cursor in word
                else {
                    if (focus_content[focus] < (int) strlen(content_new[focus])) {
                        focus_content[focus]++;
                        wmove(edit_window[focus], 0, focus_content[focus]);
                        wrefresh(edit_window[focus]);
                    }
                }
                break;
            }
            case KEY_BACKSPACE: {
                //Remove char
                if (focus < 2 && focus_content[focus] > 0) {
                    focus_content[focus]--;
                    for (int i = focus_content[focus]; content_new[focus][i] != '\0'; i++) {
                        content_new[focus][i] = content_new[focus][i + 1];
                        if (content_new[focus][i] == '\0') {
                            mvwaddch(edit_window[focus], 0, i, ' ');
                        }
                        else {
                            mvwaddch(edit_window[focus], 0, i, content_new[focus][i]);
                        }
                    }
                    wmove(edit_window[focus], 0, focus_content[focus]);
                    wrefresh(edit_window[focus]);
                }
                break;
            }
            case KEY_DC: {
                //Remove char
                if (focus < 2 && focus_content[focus] > 0) {
                    for (int i = focus_content[focus]; content_new[focus][i] != '\0'; i++) {
                        content_new[focus][i] = content_new[focus][i + 1];
                        if (content_new[focus][i] == '\0') {
                            mvwaddch(edit_window[focus], 0, i, ' ');
                        }
                        else {
                            mvwaddch(edit_window[focus], 0, i, content_new[focus][i]);
                        }
                    }
                    wmove(edit_window[focus], 0, focus_content[focus]);
                    wrefresh(edit_window[focus]);
                }
                break;
            }
            case KEY_ENTER:
            case '\n': {
                //Action if button is selected
                if (focus == 2) {
                    if (focus_button == 0) {
                        goto edit_save;
                    }
                    else if (focus_button == 1) {
                        goto edit_end;
                    }
                }
                break;
            }
            default: {
                //Add char
                if (focus < 2 && strlen(content_new[focus]) < 50) {
                    for (int i = strlen(content_new[focus]) + 1; i > focus_content[focus]; i--) {
                        content_new[focus][i] = content_new[focus][i - 1];
                        if (content_new[focus][i] != '\0') {
                            mvwaddch(edit_window[focus], 0, i, content_new[focus][i]);
                        }
                    }
                    content_new[focus][focus_content[focus]] = key_pressed;
                    mvwaddch(edit_window[focus], 0, focus_content[focus], key_pressed);
                    focus_content[focus]++;
                    wrefresh(edit_window[focus]);
                }
                break;
            }
        }
    }

    //Save edited entry to menu struct
    edit_save:
    if (menu->focus_entry == 0) {
        menu->windows[menu->focus_system].content.content[menu->focus_entry] = strdup(content_new[1]);
    }
    else {
        menu->windows[menu->focus_system].content.content[menu->focus_entry] = strdup(content_new[0]);
        menu->windows[menu->focus_system].action[menu->focus_entry] = strdup(content_new[1]);
    }
    if (deffile_update(&menu->windows[menu->focus_system]) != 0) {
        free(menu->windows[menu->focus_system].action);
        free(*menu->windows[menu->focus_system].content.content);
        free(menu->windows[menu->focus_system].content.content);
        menu->windows[menu->focus_system].content.content = NULL;
        win_content_get(&menu->windows[menu->focus_system]);
        if (!menu->windows[menu->focus_system].content.content) {
            retval = -2;
        }
        else {
            retval = -1;
        }
    }

    //Cleanup, return
    edit_end:
    curs_set(0);
    for(int i = 0; i < 5; i++) {
        delwin(edit_window[i]);
    }
    free(edit_window);
    return retval;
}