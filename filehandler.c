#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>

#include "log.h"
#include "types.h"

//Function to read content from file and return each row as an arr entry
//Returns empty LIST on fail
LIST file_read(char *filepath) {
    //Create vars, pointers. Allocate space for one char** in target.content.
    LIST target = {1, 0, malloc(sizeof(*target.content))};
    if (!target.content) {
        log_write(errno, "Error: Could not allocate memory");
        target.content = NULL;
        target.num_entries = 0;
        return target;
    }
    int *pos = (int*) malloc(50 * sizeof(*pos));
    if (!pos) {
        log_write(errno, "Error: Could not allocate memory");
        free(target.content);
        target.content = NULL;
        target.num_entries = 0;
        return target;
    }

    //Input validation
    if (filepath[0] != '/' || strlen(filepath) < 2) {
        log_write(22, "Error: Invalid filepath");
        free(pos);
        free(target.content);
        target.content = NULL;
        target.num_entries = 0;
        return target;
    }

    struct stat buffer;
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        log_write(errno, "Error: Failed to open file %s", filepath);
        free(target.content);
        free(pos);
        target.content = NULL;
        target.num_entries = 0;
        return target;
    }

    //If file found and stat-ed, allocate memory and read contents into *target.content. Set last char to \0
    int status = stat(filepath, &buffer);
    if (status == -1) {
        log_write(errno, "Error: Could not read file %s", filepath);
        free(pos);
        free(target.content);
        target.content = NULL;
        target.num_entries = 0;
        return target;
    }
    target.size_content = buffer.st_size - 1;
    *target.content = (char*) malloc(buffer.st_size);
    if (!*target.content) {
        log_write(errno, "Error: Could not allocate memory");
        free(pos);
        free(target.content);
        target.content = NULL;
        target.num_entries = 0;
        return target;
    }
    if ((long int)fread(*target.content, sizeof(char), buffer.st_size, fp) < buffer.st_size) {
        log_write(5, "Error: Incorrect file read for %s", filepath);
        free(pos);
        free(*target.content);
        free(target.content);
        target.content = NULL;
        target.num_entries = 0;
        return target;
    }
    (*target.content)[target.size_content] = '\0';
    fclose(fp);

    //Change \n to \0, count entries, save entry start index to arr
    *pos++ = 0;
    for (int i = 0; i < target.size_content; i++) {
        if ((*target.content)[i] == '\n') {
            (*target.content)[i] = '\0';
            if (target.num_entries % 50 == 0) {
                int *temp = realloc(pos, (50 * ((target.num_entries / 50) + 1)) * sizeof(*pos));
                if (!temp) {
                    log_write(errno, "Error: Could not allocate memory");
                    pos -= (target.num_entries);
                    free(pos);
                    free(*target.content);
                    free(target.content);
                    target.content = NULL;
                    target.num_entries = 0;
                    return target;
                }
                pos = temp;
            }
            *pos++ = ++i;
            target.num_entries++;
        }
    }
    pos -= (target.num_entries);

    //Allocate memory for target.content
    char **temp = (char**) realloc(target.content, target.num_entries * sizeof(*target.content));
    if (!temp) {
        log_write(errno, "Error: Could not allocate memory");
        free(pos);
        free(*target.content);
        free(target.content);
        target.content = NULL;
        target.num_entries = 0;
        return target;
    }
    else {
        target.content = temp;
    }

    //Point all target.content to start of next entry
    for (int i = 0; i < target.num_entries; i++) {
        target.content[i] = &(*target.content)[*pos++];
    }

    //Cleanup, return
    pos -= (target.num_entries);
    free(pos);
    return target;
}

//Function to read files in a directory and save to list
//Sets result->content to NULL on fail
void filelist_get(char *dir, LIST *result, bool subdir) {
    if (!result->content) {
        result->content = (char**) malloc(8);
        if (!result->content) {
            log_write(errno, "Error: Could not allocate memory");
            result->content = NULL;
            return;
        }
    }
    //Input validation
    if (dir[0] != '/' || strlen(dir) < 2) {
        log_write(22, "Error: Invalid path");
        if (result->size_content > 0) {
            free(*result->content);
        }
        free(result->content);
        result->content = NULL;
        result->num_entries = 0;
        result->size_content = 0;
        return;
    }

    DIR *local_DIR;
    struct dirent *in_file;
    int i = result->num_entries;

    local_DIR = opendir(dir);
    if (!local_DIR) {
        log_write(errno, "Error: Failed to open input directory %s", dir);
        if (result->size_content > 0) {
            free(*result->content);
        }
        result->num_entries = 0;
        result->size_content = 0;
        free(result->content);
        result->content = NULL;  
        return;
    }

    while ((in_file = readdir(local_DIR)) != NULL) {
        //Skip current directory and parent directory, 46 = '.', 0 = '\0'
        if (strcmp(in_file->d_name, ".") == 0 || strcmp(in_file->d_name, "..") == 0) {
            continue;
        }

        //Save full path
        char path[strlen(dir)+strlen(in_file->d_name)+2];
        snprintf(path, sizeof(path), "%s%s", dir, in_file->d_name);
        struct stat buffer;
        int status;
        
        //Stat current file
        status = stat(path, &buffer);
        if (status == -1) {
            log_write(errno, "Error: Failed to stat %s", path);
            continue;
        }

        //If regular file, increase num_entries, size_content and copy path into array. Allocate more memory for ** if needed
        if (buffer.st_mode & __S_IFREG) {
            result->num_entries++;
            if (result->num_entries % 10 == 1) {
                char **temp = (char**) realloc(result->content, (result->num_entries + 9) * sizeof(*result->content));
                if (!temp) {
                    if (result->size_content > 0) {
                        free(*result->content);
                    }
                    result->num_entries = 0;
                    result->size_content = 0;
                    free(result->content);
                    result->content = NULL;  
                    return;
                }
                result->content = temp;
            }
            result->size_content += strlen(path) + 1;
            result->content[i++] = strdup(path);
        }
        //If directory, recursively call function
        else if (subdir && (buffer.st_mode & __S_IFDIR)) {
            strcat(path, "/");
            filelist_get(path, result, true);
            if (!result->content) {
                log_write(-1, "Error: Filepath reading failed for %s", path);
                return;
            }
            i = result->num_entries;
        }
    }
    closedir(local_DIR);
    return;
}

//Write content to path, overwriting all info. Single string.
//Returns 0 on success
int file_write_str(char *path, char *content) {
    FILE *fp = fopen(path, "w");
    if (!fp) {
        int temp = errno;
        log_write(temp, "Error: Could not open file %s", path);
        return temp;
    }
    fprintf(fp, content);
    fclose(fp);
    return 0; 
}

//Write content to path, overwriting all info. Multiple strings.
//Returns 0 on success
int file_write_LIST(char *path, LIST content) {
    FILE *fp = fopen(path, "w");
    if (!fp) {
        int temp = errno;
        log_write(temp, "Error: Could not open file %s", path);
        return temp;
    }
    for (int i = 0; i < content.num_entries; i++) {
        char tmp[256];
        snprintf(tmp, sizeof(tmp), "%s\n", content.content[i]);
        fprintf(fp, tmp);
    }
    fclose(fp);
    return 0;
}

//Sort list contents alphabetically
void LIST_sort(LIST *content) {
    char *tmp = NULL;
    for (int i = 1; i < content->num_entries; i++) {
        int j = i;
        while (j > 0 && strcasecmp(content->content[i], content->content[j - 1]) < 0) {
            j--;
        }
        if (i == j) {
            continue;
        }
        tmp = content->content[i];
        for (int pos = i; pos > j; pos--) {
            content->content[pos] = content->content[pos - 1];
        }
        content->content[j] = tmp;
    }
}

//Read content from file and put it into WIN LIST section
//On fail, set window->content.content to NULL
void win_content_get(WIN *window) {
    //Allocate and fill content
    window->content = file_read(window->deffile);
    if (!window->content.content) {
        log_write(5, "Error: could not get LIST from %s", window->deffile);
        return;
    }
    window->action = (char**) malloc(window->content.num_entries * sizeof(*window->action));
    if (!window->action) {
        log_write(errno, "Error: Could not allocate memory");
        free(*window->content.content);
        free(window->content.content);
        window->content.content = NULL;
        return;
    }

    //Special logic for main menu
    char filename[11];
    int startindex = strlen(window->deffile) - 10;
    strncpy(filename, window->deffile + startindex, 11);
    if (strcmp(filename, "main.entry") == 0) {
        for (int j = 0; window->content.content[0][j] != '\0'; j++){
            if (window->content.content[0][j] == '|') {
                window->content.content[0][j++] = '\0';
                window->action[0] = &window->content.content[0][j];
                break;
            }
        }
    }
    window->gamedir = &window->content.content[0];

    //Point action to second part of content string
    for (int i = 1; i < window->content.num_entries; i++) {
        for (int j = 0; window->content.content[i][j] != '\0'; j++){
            if (window->content.content[i][j] == '|') {
                window->content.content[i][j++] = '\0';
                window->action[i] = &window->content.content[i][j];
                break;
            }
        }
    }

    //Decide window width
    window->w = 0;
    for (int i = 1; i < window->content.num_entries; i++){
        if ((int)strlen(window->content.content[i]) > window->w) {
            window->w = strlen(window->content.content[i]);
        }
    }
    window->w += 2;
    if (window->w < 6) {
        window->w = 6;
    }
    else if (getmaxx(stdscr) < window->w) {
        window->w = getmaxx(stdscr);
    }
    if (strcmp(filename, "main.entry") == 0 && window->w < 14) {
        window->w = 14;
    }
    if (window->win) {
        wresize(window->win, window->h, window->w);
    }
}

//Update content of menu item definition file
//Returns 0 on success
int deffile_update(WIN *window) {
    //Input validation
    if (!window->deffile) {
        log_write(22, "Error: Filepath not defined");
        return 22;
    }
    if (window->deffile[0] != '/' || strlen(window->deffile) < 2) {
        log_write(22, "Error: Invalid file path");
        return 22;
    }

    LIST newfile = {0, 0, NULL};
    newfile.num_entries = window->content.num_entries;
    newfile.content = (char**) malloc(window->content.num_entries * sizeof(*newfile.content));
    if (!newfile.content) {
        int temp = errno;
        log_write(temp, "Error: Could not allocate memory");
        return temp;
    }

    newfile.content[0] = window->content.content[0];
    for (int i = 1; i < newfile.num_entries; i++){
        char tmpres[256];
        snprintf(tmpres, sizeof(tmpres), "%s|%s", window->content.content[i], window->action[i]);
        newfile.content[i] = strdup(tmpres);
        newfile.size_content += strlen(tmpres) + 1;
    }

    char *tmp = NULL;
    for (int i = 3; i < window->content.num_entries; i++) {
        int j = i;
        while (j > 2 && strcasecmp(newfile.content[i], newfile.content[j - 1]) < 0) {
            j--;
        }
        if (i == j) {
            continue;
        }
        tmp = newfile.content[i];
        for (int pos = i; pos > j; pos--) {
            newfile.content[pos] = newfile.content[pos - 1];
        }
        newfile.content[j] = tmp;
    }

    int retval = file_write_LIST(window->deffile, newfile);
    free(newfile.content);

    free(window->action);
    free(*window->content.content);
    free(window->content.content);
    window->content.content = NULL;
    win_content_get(window);
    if (!window->content.content) {
        return -2;
    }
    return retval;
}

//Function for making a list with game name and shortpath from filelist
//Returns 0 on success
int gamelist_parse(LIST *filelist, WIN *window, char *typefile) {
    //Input validation
    if (!filelist->content) {
        log_write(22, "Error: List is empty");
        return 22;
    }
    if (typefile[0] != '/' || strlen(typefile) < 2) {
        log_write(22, "Error: Invalid filepath");
        return 22;
    }

    int num_entries = filelist->num_entries;
    int resindex = 0;
    char tmpname[filelist->num_entries][64], tmppath[filelist->num_entries][192];
    for (int j = 0; j < filelist->num_entries; j++){
        strcpy(tmpname[j], "");
    }
    LIST types = file_read(typefile);
    if (!types.content) {
        return -1;
    }

    //For each entry, check that it has a proper file ending and get start and end of filename without filetype
    for (int i = 0; i < filelist->num_entries; i++) {
        char *filetype = strdup(filelist->content[i] + (strlen(filelist->content[i]) - 4));
        bool badfiletype = true;
        for (int j = 0; j < types.num_entries; j++) {
            if (strcmp(filetype, types.content[j]) == 0) {
                badfiletype = false;
                break;
            }
        }
        if (badfiletype) {
            num_entries--;
            continue;
        }
        int startpos = 0, endpos = 0;
        for (int j = strlen(filelist->content[i]) - 1; j > 0; j--) {
            if (filelist->content[i][j] == '.' && endpos == 0) {
                endpos = j--;
            }
            if (filelist->content[i][j] == '/') {
                startpos = j + 1;
                break;
            }
        }
        
        //Copy name and path into temp variables, copy name from window if preexisting
        strcpy(tmppath[resindex], filelist->content[i] + strlen(*window->gamedir));
        for (int j = 2; j < window->content.num_entries; j++) {
            if (strcmp(window->action[j], tmppath[resindex]) == 0) {
                strcpy(tmpname[resindex], window->content.content[j]);
                break;
            }
        }
        if(tmpname[resindex][0] == '\0') {
            strncpy(tmpname[resindex], filelist->content[i] + startpos, endpos - startpos);
            tmpname[resindex][endpos - startpos] = '\0';
        }
        resindex++;
    }
    free(*types.content);
    free(types.content);
    types.content = NULL;

    //Save gamedir and emu info, clear window entries
    char tmpgamedir[256], tmpemuname[256], tmpemupath[256];
    strcpy(tmpgamedir, window->content.content[0]);
    strcpy(tmpemuname, window->content.content[1]);
    strcpy(tmpemupath, window->action[1]);
    free(*window->content.content);
    free(window->content.content);
    free(window->action);
    window->content.content = malloc((num_entries + 2) * sizeof(*window->content.content));
    if (!window->content.content) {
        log_write(errno, "Error: Could not allocate memory");
        return -2;
    }
    window->action = malloc((num_entries + 2) * sizeof(*window->action));
    if (!window->action) {
        log_write(errno, "Error: Could not allocate memory");
        free(window->content.content);
        window->content.content = NULL;
        return -2;
    }

    //Refill window entries
    window->content.content[0] = strdup(tmpgamedir);
    window->content.content[1] = strdup(tmpemuname);
    window->action[1] = strdup(tmpemupath);
    window->content.num_entries = num_entries + 2;
    for (int i = 0; i < num_entries; i++) {
        window->content.content[i + 2] = strdup(tmpname[i]);
        window->action[i + 2] = strdup(tmppath[i]);
    }
    return 0;
}

//Function to scan gamedir for games and update list
//Returns 0 on success
int scan(WIN *window, char *typefile) {
    //Input validation
    if (typefile[0] != '/' || strlen(typefile) < 2) {
        log_write(22, "Error: Invalid filepath");
        return 22;
    }

    LIST scanlist = {0, 0, NULL};
    filelist_get(*window->gamedir, &scanlist, true);
    if (!scanlist.content) {
        return -1;
    }

    //Create list with game name and shortpath
    if (gamelist_parse(&scanlist, window, typefile) != 0) {
        free(*scanlist.content);
        free(scanlist.content);
        scanlist.content = NULL;
        return -1;
    }
    if (deffile_update(window) != 0) {
        free(*scanlist.content);
        free(scanlist.content);
        scanlist.content = NULL;
        return -1;
    }

    //Free allocated memory that is no longer needed
    free(*scanlist.content);
    free(scanlist.content);
    scanlist.content = NULL;
    return 0;
}

//Function for parsing environment variables
int resolve_env(char **path) {
    char envvar[32];
    envvar[0] = '\0';
    int pathbp = 0;
    for (int i = 2; i < (int) strlen(*path); i++) {
        if ((*path)[i] == '/') {
            pathbp = i;
            strncpy(envvar, *path + 1, --i);
            envvar[i] = '\0';
            break;
        }
    }
    char *envpath = getenv(envvar);
    if(!envpath) {
        return 1;
    }
    char newpath[128];
    snprintf(newpath, sizeof(newpath), "%s%s", envpath, *path + pathbp);
    *path = strdup(newpath);
    return 0;
}