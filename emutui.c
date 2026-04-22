#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/joystick.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

#include "config.h"
#include "drw.h"
#include "filehandler.h"
#include "log.h"
#include "types.h"

int main() {
    //Resolve paths from config
    if (logfile[0] == '$') {
        if (resolve_env(&logfile) != 0) {
            perror("Fatal: Failed to resolve environment variable");
            return 1;
        }
    }
    if (menudir[0] == '$') {
        resolve_env(&menudir);
    }
    if (lastgamefile[0] == '$') {
        resolve_env(&lastgamefile);
    }
    if (scantypefile[0] == '$') {
        resolve_env(&scantypefile);
    }

    //Starting logging, NCurses
    if (log_init(logfile) != 0) {
        fprintf(stderr, "Fatal: Could not open logfile %s - %s", logfile, strerror(errno));
        return errno;
    }
    log_write(0, "Info: Initializing NCurses");
    if (!initscr()) {
        log_write(-1, "Fatal: Failed to initialize screen");
        log_close();
        return 1;
    }

    //NCurses setup
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, true);
    nodelay(stdscr, true);
    if (!has_colors()) {
        endwin();
        log_write(-1, "Fatal: Your terminal doesn't support colour");
        log_close();
        return 1;
    }
    start_color();
    init_pair(1, color_text, color_cursor);
    log_write(0, "Info: NCurses initialized");

    //Read files in menu dir
    log_write(0, "Info: Checking for emulators");
    LIST menu_files = {0, 0, NULL};
    filelist_get(menudir, &menu_files, false);
    if (!menu_files.content) {
        endwin();
        log_write(5, "Fatal: Could not read directory %s", menudir);
        log_close();
        return 1;
    }
    log_write(0, "Info: Found %d menu entries", menu_files.num_entries - 1);

    //Create windows for menu contents
    log_write(0, "Info: Populating the menu");
    MENU menu = {0, 0, 0, getmaxy(stdscr), getmaxx(stdscr), NULL};
    if (menu.scrmaxx < 14) {
        log_write(95, "Fatal: Screen is too narrow");
        free(*menu_files.content);
        free(menu_files.content);
        log_close();
        return 1;
    }
    menu.windows = (WIN*) malloc(menu_files.num_entries * sizeof(*menu.windows));
    if (!menu.windows) {
        int temp = errno;
        endwin();
        log_write(temp, "Fatal: Could not allocate memory");
        free(*menu_files.content);
        free(menu_files.content);
        log_close();
        return 1;
    }

    int index = 1;
    for (int i = 0; i < menu_files.num_entries; i++) {
        char filename[11];
        int startindex = strlen(menu_files.content[i]) - 10;
        strncpy(filename, menu_files.content[i] + startindex, 11);
        if (strcmp(filename, "main.entry") == 0) {
            menu.windows[0] = win_create(menu.scrmaxy, 0, 0, menu_files.content[i]);
            if (!menu.windows[0].win) {
                endwin();
                log_write(5, "Fatal: Could not create window for main menu");
                free(*menu_files.content);
                free(menu_files.content);
                log_close();
                return 1;
            }
            menu.num_win++;
            continue;
        }
        else if (strcmp(menu_files.content[i] + (strlen(menu_files.content[i]) - 6), ".entry") == 0) {
            menu.windows[index] = win_create(menu.scrmaxy, 0, 0, menu_files.content[i]);
            if (menu.windows[index].win) {
                index++;
                menu.num_win++;
            }
            else {
                log_write(5, "Error: Could not create window for %s", menu_files.content[i]);
            }
        }
    }
    log_write(0, "Info: Menu populated");

    //Deallocate filelist from menu dir
    free(*menu_files.content);
    free(menu_files.content);
    menu_files.content = NULL;

    //Fill and draw windows, set initial position
    log_write(0, "Info: Drawing the TUI");
    while (menu.focus_system < menu.num_win) {
        if (menu.focus_system == 0) {
            menu_scroll(&menu, true);
            menu.focus_entry = 1;
        }
        else {
            menu_scroll(&menu, false);
        }
        menu.focus_system++;
    }
    menu.focus_system = 0;
    menu.focus_entry = 0;
    menu_shift(&menu);

    //Setup for menu logic
    pid_t pid;
    bool edit_flag = FALSE, kill_flag = FALSE, fork_flag = FALSE;
    char emuinfo[3][256];
    int js = open("/dev/input/js0", O_RDONLY | O_NONBLOCK);
    if (js == -1) {
        log_write(errno, "Warning: Device js0 could not be opened");
    }
    int last_kp = ERR;
    __u8 last_bp = 200;
    time_t js_check = 0;

    //Logic for menu navigation
    while (true) {
        //Check for keypresses and controller events
        int key_pressed = -1;
        int kb_event = getch();
        if (kb_event != last_kp) {
            key_pressed = kb_event;
            last_kp = kb_event;
        }
        if (js < 0) {
            time_t now = time(NULL);
            if (now - js_check >= 5) {
                js = open("/dev/input/js0", O_RDONLY | O_NONBLOCK);
                js_check = now;
                if (js < -1) {
                    log_write(0, "Info: Device js0 found");
                }
            }
        }
        struct js_event event;
        if (read(js, &event, sizeof(event)) > 0) {
            if (event.value == 1 && event.number != last_bp) {
                key_pressed = (int) event.number;
                last_bp = event.number;
            }
            else if (event.value == 0 && event.number == last_bp) {
                last_bp = 200;
            }
        }

        //Check which key/button was pressed, if any
        switch (key_pressed) {
        case KEY_LEFT:
        case X360_BTN_LEFT:
            //Shift focus to the window to the left, redraw screen
            if (menu.focus_system > 0) {
                menu.focus_entry = 1;
                menu_scroll(&menu, false);
                menu.focus_system--;
                if (menu.focus_system == 0) {
                    menu.focus_entry = 0;
                }
                menu_shift(&menu);
                menu_scroll(&menu, true);
            }
            break;
        case KEY_RIGHT:
        case X360_BTN_RIGHT:
            //Shift focus to the window to the right, redraw screen
            if (menu.focus_system < (menu.num_win - 1)) {
                if (menu.focus_system == 0) {
                    menu.focus_entry = 0;
                }
                else {
                    menu.focus_entry = 1;
                }
                menu_scroll(&menu, false);
                menu.focus_system++;
                if (menu.focus_entry == 0) {
                    menu.focus_entry = 1;
                }
                menu_shift(&menu);
                menu_scroll(&menu, true);
            }
            break;
        case KEY_UP:
        case X360_BTN_UP:
            //Shift focus one menu entry up, redraw window
            if (menu.focus_entry > 0) {
                menu.focus_entry--;
                menu_scroll(&menu, true);
            }
            break;
        case KEY_DOWN:
        case X360_BTN_DOWN:
            //Shift focus one menu entry down, redraw window
            if (menu.focus_entry < (menu.windows[menu.focus_system].content.num_entries - 1)) {
                menu.focus_entry++;
                menu_scroll(&menu, true);
            }
            break;
        case '\n':
        case KEY_ENTER:
        case X360_BTN_A:
            //If in main menu
            if (menu.focus_system == 0) {
                switch (menu.windows[menu.focus_system].action[menu.focus_entry][2]) {
                //If Resume
                case 'R': {
                    log_write(0, "Info: Resuming last played game");
                    //Get last game played from file
                    LIST lastgame = file_read(lastgamefile);
                    if (!lastgame.content || lastgame.num_entries != 1) {
                        log_write(-1, "Error: Failed to read file %s", lastgamefile);
                        if (remove(lastgamefile) != 0) {
                            log_write(errno, "Warning: Could not delete file %s", lastgamefile);
                        }
                        break;
                    }

                    //If file is not missing, empty, or contains more than one entry
                    int bp = 0, pos = 0;
                    int length = (int) strlen(lastgame.content[0]);
                    for (int i = 0; i <= length; i++){
                        if (lastgame.content[0][i] == '|') {
                            lastgame.content[0][i] = '\0';
                            strncpy(emuinfo[pos++], lastgame.content[0] + bp, i + 1);
                            bp = ++i;
                        }
                        else if (lastgame.content[0][i] == '\0') {
                            strncpy(emuinfo[pos], lastgame.content[0] + bp, i);
                            break;
                        }
                    }
                    //Check that game file and emulator exists
                    if (access(emuinfo[0], F_OK) != 0) {
                        log_write(errno, "Error: Could not find emulator %s", emuinfo[1]);
                        if (remove(lastgamefile) != 0) {
                            log_write(errno, "Warning: Could not delete file %s", lastgamefile);
                        }
                        break;
                    }
                    if (pos == 2) {
                        if (access(emuinfo[2], F_OK) != 0){
                            log_write(errno, "Error: Could not find game file %s", emuinfo[2]);
                            if (remove(lastgamefile) != 0) {
                                log_write(errno, "Warning: Could not delete file %s", lastgamefile);
                            }
                            break;
                        }
                    }
                    else {
                        strcpy(emuinfo[2], emuinfo[0]);
                    }

                    free(*lastgame.content);
                    free(lastgame.content);
                    lastgame.content = NULL;

                    //Launch emulator if everything checks out
                    goto emu_start;
                }
                //IF Scan all
                case 'U': {
                    log_write(0, "Info: Scanning all directories for games");
                    int scanall_entry = menu.focus_entry;
                    menu.focus_entry = 1;
                    menu.focus_system = 1;
                    while (menu.focus_system < menu.num_win) {
                        if (scan(&menu.windows[menu.focus_system], scantypefile) != 0) {
                            log_write(-1, "Error: Failed to scan %s for games", menu.windows[menu.focus_system].gamedir);
                        }
                        menu_scroll(&menu, false);
                        menu.focus_system++;
                    }
                    menu.focus_system = 0;
                    menu.focus_entry = scanall_entry;
                    menu_shift(&menu);
                    log_write(0, "Info: All directories scanned");
                    break;
                }
                //If Edit
                case 'E': {
                    if (edit_flag) {
                        log_write(0, "Info: Exiting edit mode");
                        edit_flag = FALSE;
                        menu.windows[menu.focus_system].content.content[menu.focus_entry] = strdup("Edit");
                        menu_scroll(&menu, true);
                        menu_shift(&menu);
                    }
                    else {
                        log_write(0, "Info: Entering edit mode");
                        edit_flag = TRUE;
                        menu.windows[menu.focus_system].content.content[menu.focus_entry] = strdup("Stop editing");
                        menu_scroll(&menu, true);
                        menu_shift(&menu);
                    }
                    break;
                }
                //If Quit
                case 'Q':
                    log_write(0, "Info: Exiting EmuTUI");
                    goto program_end;
                //If Shutdown
                case 'S':
                    log_write(0, "Info: Shutting down system");
                    kill_flag = TRUE;
                    goto program_end;
                }
            }
            //If in edit mode
            else if (edit_flag) {
                log_write(0, "Info: Editing %s", menu.windows[menu.focus_system].content.content[menu.focus_entry]);
                int edit_retval = entry_edit(&menu);
                if (edit_retval == -2) {
                    log_write(-2, "Error: Failed to edit %s, emulator %s removed from listing due to this failure", menu.windows[menu.focus_system].content.content[menu.focus_entry], menu.windows[menu.focus_system].content.content[1]);
                }
                else if (edit_retval != 0) {
                    log_write(-1, "Error: Failed to edit %s, no action taken", menu.windows[menu.focus_system].content.content[menu.focus_entry]);
                }
                log_write(0, "Info: %s edited", menu.windows[menu.focus_system].content.content[menu.focus_entry]);
                menu_scroll(&menu, true);
                menu_shift(&menu);
            }
            //If Scan
            else if (menu.focus_entry == 0) {
                log_write(0, "Info: Scanning %s for games", *menu.windows[menu.focus_system].gamedir);
                if (scan(&menu.windows[menu.focus_system], scantypefile) != 0) {
                    log_write(-1, "Error: Failed to scan %s for games", *menu.windows[menu.focus_system].gamedir);
                }
                log_write(0, "Info: %s scanned for games", *menu.windows[menu.focus_system].gamedir);
                menu_scroll(&menu, true);
                menu_shift(&menu);
            }
            //If emulator or game
            else {
                //Copy relevant info to array for easy access and streamline with other functions
                strcpy(emuinfo[0], menu.windows[menu.focus_system].action[1]);
                strcpy(emuinfo[1], menu.windows[menu.focus_system].content.content[1]);
                snprintf(emuinfo[2], sizeof(emuinfo[2]), "%s%s", *menu.windows[menu.focus_system].gamedir, menu.windows[menu.focus_system].action[menu.focus_entry]);
                
                //Code to launch emulator and stop EmuTUI
                emu_start:
                log_write(0, "Info: Starting emulation");
                endwin();
                //Clear buffer top avoid duplicated logs
                fflush(NULL);
                fork_flag = TRUE;
                pid = fork();
                //If current process is child process
                if (pid == 0) {
                    char *gamename = strdup(menu.windows[menu.focus_system].content.content[menu.focus_entry]);
                    //Cleanup
                    for (int i = 0; i < menu.num_win; i++) {
                        free(menu.windows[i].action);
                        free(*menu.windows[i].content.content);
                        free(menu.windows[i].content.content);
                        menu.windows[i].content.content = NULL;
                        delwin(menu.windows[i].win);
                    }
                    free(menu.windows);

                    //Start emulator/game
                    if (menu.focus_entry != 1 && strcmp(emuinfo[0], emuinfo[2]) != 0) {
                        log_write(0, "Info: Launching %s on %s", gamename, emuinfo[1]);
                        log_close();
                        execl(emuinfo[0], emuinfo[1], emuinfo[2], NULL);
                    }
                    else {
                        log_write(0, "Info: Launching %s", emuinfo[1]);
                        log_close();
                        execl(emuinfo[0], emuinfo[1], NULL);
                    }
                    perror("Fatal: could not start emulator");
                    return 1;
                }
                //If current process is parent process
                else if (pid > 0) {
                    char *curgame = NULL;
                    //Save game to file for use in menu action Resume
                    log_write(0, "Info: Updating %s", lastgamefile);
                    //If emulator was launched
                    if (menu.focus_entry == 1 || strcmp(emuinfo[0], emuinfo[2]) == 0) {
                        int length = strlen(emuinfo[0]) + strlen(emuinfo[1]) + 3;
                        curgame = (char*) malloc(length * sizeof(*curgame));
                        if (curgame) {
                            snprintf(curgame, length, "%s|%s\n", emuinfo[0], emuinfo[1]);
                        }
                        else {
                            log_write(errno, "Error: Could not allocate memory");
                        }
                    }
                    //If game was launched
                    else {
                        int length = strlen(emuinfo[0]) + strlen(emuinfo[1]) + strlen(emuinfo[2]) + 4;
                        curgame = (char*) malloc(length * sizeof(*curgame));
                        if (curgame) {
                            snprintf(curgame, length, "%s|%s|%s\n", emuinfo[0], emuinfo[1], emuinfo[2]);
                        }
                        else {
                            log_write(errno, "Error: Could not allocate memory");
                        }
                    }
                    //Write to file
                    if (curgame){
                        if (file_write_str(lastgamefile, curgame) == 0) {
                            log_write(0, "Info: %s updated", lastgamefile);
                        }
                        else {
                            log_write(-1, "Error: Failed to update last game played");
                        }
                        free(curgame);
                        curgame = NULL;
                    }
                    goto program_end;
                }
                else {
                    log_write(errno, "Fatal: Could not fork process");
                    goto program_end;
                }
            }
        }
        //Wait in order to handle key- and buttonpresses without blocking
        napms(50);
    }

    //End program
    program_end:
    //Cleanup
    endwin();
    for (int i = 0; i < menu.num_win; i++) {
        free(menu.windows[i].action);
        free(*menu.windows[i].content.content);
        free(menu.windows[i].content.content);
        menu.windows[i].content.content = NULL;
        delwin(menu.windows[i].win);
    }
    free(menu.windows);
    if (fork_flag && pid > 0) {
        //Wait for emulator to exit
        sleep(1);
        char emu[strlen(emuinfo[1]) + 7];
        strcpy(emu, "pgrep ");
        for (int i = 0; i < (int) strlen(emu); i++) {
            emu[i + 6] = tolower(emuinfo[1][i]);
        }
        FILE *pgrep = popen(emu, "r");
        if (!pgrep) {
            log_write(errno, "Warning: popen failed");
        }
        else {
            pid_t emu_pid;
            //If no process is found, end
            if (fscanf(pgrep, "%d", &emu_pid) != 1) {
                log_write(errno, "Warning: No emulator process found");
                pclose(pgrep);
            }
            //Else, wait for process to exit
            else {
                while (kill(emu_pid, 0) == 0) {
                    sleep(0.5);
                }
                log_write(0, "Info: Emulation ended");
            }
        }
        //Restart program
        log_write(0, "Info: Restarting EMUtui");
        log_close();
        execl("/proc/self/exe", "emutui", NULL);
        perror("Fatal: Failed to relaunch after emulator exit");
        return 1;
    }
    log_close();
    //If shutdown was chosen
    if (kill_flag) {
        system("/sbin/shutdown -P now");
    }
    return 0;
}
