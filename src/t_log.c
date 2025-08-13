#include <stdio.h>
#include <stdlib.h>

FILE *logFile;

void t_log(char *text) {

  if (logFile == NULL) {
    char path[1024];
    sprintf(path, "%s/logs/tiny-audio.txt", getenv("HOME"));
    logFile = fopen(path, "a");
    fprintf(logFile, "---\n");
  }
  fprintf(logFile, "%s\n", text);
}
