#ifndef LOG_H
#define LOG_H

int log_write(int error, char *message, ...);
int log_init(char *logfile);
int log_close();

#endif