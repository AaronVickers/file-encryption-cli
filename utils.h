// Required headers
#include <stdbool.h>
#include <stdio.h>

bool isFile(char *filePath);
bool isDirectory(char *directoryPath);

long getFileSize(FILE *fp);