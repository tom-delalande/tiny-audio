#include <stdio.h>

FILE *logFile;

void t_log(char *text) {
  if (logFile == NULL) {
    // FIXME: This does not work
    logFile = fopen("./logs.txt", "a");
  }
  fprintf(logFile, "%s", text);
}
