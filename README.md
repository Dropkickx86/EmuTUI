<h1>EmuTUI</h1>
This is a simple TUI-based launcher for emulators. No emulator or game is included, this is only a launcher for emulators acquired elsewhere.

The project should compile and run on all glibc-based POSIX-compliant distros. Compilation and running on musl has been successfully tested, but is not guaranteed to work for all versions. The only dependency past libc is NCurses.

Emulators are added to the launcher by creating files in the config directory (default $HOME/.config/emutui/menu/) with the ending ".entry". The files should be in the format
```
<Path to directory with games>
<Emulator name>|<Path to emulator binary>
<Game name>|<Path to game file>
<Game name>|<Path to game file>
...
<Game name>|<Path to game file>
```
The paths to game directory and emulator binary have to be absolute. The paths to the games can be absolute or be relative to the game directory. Example:
```
/storage/games/emulation/PS2/ISO/
PCSX2|/usr/local/bin/pcsx2
Crash Tag Team Racing|CTTR.iso
Donald Duck Goin' Quackers|Donald Duck Goin' Quackers.iso
Flatout 2|/usr/local/games/flatout2.iso
```
Emulators and games can also be added directly in the TUI, and games in the game directory can be added automatically with the scan function.

Filetypes for games found with the scan function are defined in the scan.types file (default $HOME/.config/emutui/scan.types). If an emulator uses a file format that is not recognized it only needs to be added to the file. Please also open an issue where the emulator and filetype is defined and it can be added to the next commit.

Configuration of file paths is done via config.h and config.mk and requires building the project for all changes.

If compiled with the option TTY=1, the program will launch the emulator in a program defined in config.h. This is designed for the possibility to run directly on TTY but open emulators under Wayland/X11.

The program has basic controller support and is preconfigured to work with a Xbox360 controller under Debian. Mapping is done in config.h.
