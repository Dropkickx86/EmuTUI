#ifndef CONFIG_H_
#define CONFIG_H_

#define X360_BTN_DOWN 14
#define X360_BTN_UP 13
#define X360_BTN_LEFT 11
#define X360_BTN_RIGHT 12
#define X360_BTN_A 0
#define X360_BTN_B 1
#define X360_BTN_X 2
#define X360_BTN_Y 3
#define X360_BTN_START 7

#ifdef TTY
    static char *wm_path = "/usr/bin/cage";
#endif

static char *logfile = "$HOME/.config/emutui/emutui.log";
static char *menudir = "$HOME/.config/emutui/menu/";
static char *lastgamefile = "$HOME/.config/emutui/lastgame.info";
static char *scantypefile = "$HOME/.config/emutui/scan.types";
static int color_text = COLOR_WHITE;
static int color_cursor = COLOR_RED;

#endif