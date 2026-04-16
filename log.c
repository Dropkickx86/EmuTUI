#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//Global pointer to log file
FILE *logfileptr;

//Function for writing a message to log file
//Returns 0 on success
int log_write(int error, char *message, ...) {
    if (!logfileptr) {
        return 5;
    }
    time_t now = time(NULL);
    struct tm tm = *localtime(&now);
    fprintf(logfileptr, "%d-%02d-%02d %02d:%02d:%02d | ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    va_list args;
    va_start(args, message);
    vfprintf(logfileptr, message, args);
    if (error > 0 && error <= 133) {
        fprintf(logfileptr, " - %s\n", strerror(error));
    }
    else {
        fprintf(logfileptr, "\n");
    }
    va_end(args);
    return 0;
}

//Function for opening the log file
//Returns 0 on success
int log_init(char *logfile) {
    logfileptr = fopen(logfile, "a");
    if (!logfileptr) {
        return errno;
    }
    return 0;
}

//Function for closing the log file
//Returns 0 on success
int log_close() {
    fclose(logfileptr);
    return 0;
}