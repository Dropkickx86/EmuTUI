#include "types.h"

LIST file_read(char *filepath);
void filelist_get(char *dir, LIST *result, bool subdir);
int file_write_str(char *path, char *content);
int FILE_sort(FILE *content);
void win_content_get(WIN *window);
int deffile_update(WIN *system);
int gamelist_parse(LIST *filelist, WIN *window, char *typefile);
int scan(WIN *window, char *typefile);
int resolve_env(char **path);