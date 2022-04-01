// Include header file
#include "utils.h"

// Required headers
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>

bool isFile(char *filePath) {
    // Buffer for stats
    struct stat statBuffer;

    // Check if regular file
    if (lstat(filePath, &statBuffer) == 0 && S_ISREG(statBuffer.st_mode)) {
        // Is a file
        return true;
    }

    // Not a file
    return false;
}

bool isDirectory(char *directoryPath) {
    // Buffer for stats
    struct stat statBuffer;

    // Check if directory
    if (lstat(directoryPath, &statBuffer) == 0 && S_ISDIR(statBuffer.st_mode)) {
        // Is a directory
        return true;
    }

    // Not a directory
    return false;
}

long getFileSize(FILE *fp) {
    // Get initial file position
    long initialPosition = ftell(fp);

    // Move to end of file
    fseek(fp, 0, SEEK_END);

    // Get current (end of file) position
    long endPosition = ftell(fp);

    // Reset to initial file position
    fseek(fp, initialPosition, SEEK_SET);
    
    // Return end of file position
    return endPosition;
}