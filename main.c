// Required headers
#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

// Include AES encryption header
#include "aes.h"

// Initialization vector size
#define IV_SIZE 16

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

uint8_t *generateIV(int length) {
    // Allocate buffer for IV
    uint8_t *randomIV = (uint8_t*) malloc(length * sizeof(uint8_t));

    // Open urandom file
    FILE *fp = fopen("/dev/urandom", "r");

    // Store number of bytes read
    size_t bytesRead = 0;

    // Loop while bytes read less than length
    while (bytesRead < length) {
        // Increase number of bytes read and add them to buffer
        bytesRead += fread(randomIV, 1, length-bytesRead, fp);
    }

    // Close urandom file
    fclose(fp);
    
    // Return IV buffer
    return randomIV;
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

void encryptFile(char *filePath, uint8_t *key) {
    // Open unencrypted file in read mode
    FILE *fp = fopen(filePath, "r");

    // Get file size
    long fileSize = getFileSize(fp);

    // Return if file size invalid
    if (fileSize < 1) {
        return;
    }

    // Allocate IV and file content buffers
    uint8_t *fileIV = generateIV(IV_SIZE);
    char *fileBuffer = (char*) malloc(sizeof(char) * fileSize);

    // Read file content into buffer and close file
    fread(fileBuffer, 1, fileSize, fp);
    fclose(fp);

    // Encrypt buffer using AES-CTR, IV, and key
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, fileIV);
    AES_CTR_xcrypt_buffer(&ctx, fileBuffer, fileSize);

    // Open unencrypted file in write mode
    fp = fopen(filePath, "w");

    // Write IV and encrypted content and close file
    fwrite(fileIV, 1, IV_SIZE, fp);
    fwrite(fileBuffer, 1, fileSize, fp);
    fclose(fp);

    // Free allocated buffers
    free(fileIV);
    free(fileBuffer);
}

void decryptFile(char *filePath, uint8_t *key) {
    // Open encrypted file in read mode
    FILE *fp = fopen(filePath, "r");

    // Get file size
    long fileSize = getFileSize(fp);

    // Return if file size invalid
    if (fileSize <= IV_SIZE) {
        return;
    }

    // Allocate IV and file content buffers
    uint8_t *fileIV = (uint8_t*) malloc(sizeof(uint8_t) * IV_SIZE);
    char *fileBuffer = (char*) malloc(sizeof(char) * (fileSize - IV_SIZE));

    // Read IV and file content into buffers and close file
    fread(fileIV, 1, IV_SIZE, fp);
    fread(fileBuffer, 1, (fileSize - IV_SIZE), fp);
    fclose(fp);

    // Decrypt buffer using AES-CTR, IV, and key
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, fileIV);
    AES_CTR_xcrypt_buffer(&ctx, fileBuffer, (fileSize - IV_SIZE));

    // Open encrypted file in write mode
    fp = fopen(filePath, "w");

    // Write unencrypted content and close file
    fwrite(fileBuffer, 1, (fileSize - IV_SIZE), fp);
    fclose(fp);

    // Free allocated buffers
    free(fileIV);
    free(fileBuffer);
}

void encryptDirectory(char *directoryPath, uint8_t *key) {
    // Open directory and create buffer for child path
    DIR *dp = opendir(directoryPath);
    struct dirent *child;
    char childPath[PATH_MAX + 1];

    // Loop over all directory entries
    while ((child = readdir(dp)) != NULL) {
        // Ignore current and parent directory entries
        if (strcmp(child->d_name, ".") == 0 || strcmp(child->d_name, "..") == 0) {
            continue;
        }

        // Append child name to directory path
        sprintf(childPath, "%s/%s", directoryPath, child->d_name);

        // Handle child type
        if (child->d_type == DT_REG) {
            // Encrypt file
            encryptFile(childPath, key);
        } else if (child->d_type == DT_DIR) {
            // Encrypt directory
            encryptDirectory(childPath, key);
        }
    }

    // Close directory
    closedir(dp);
}

void decryptDirectory(char *directoryPath, uint8_t *key) {
    // Open directory and create buffer for child path
    DIR *dp = opendir(directoryPath);
    struct dirent *child;
    char childPath[PATH_MAX + 1];

    // Loop over all directory entries
    while ((child = readdir(dp)) != NULL) {
        // Ignore current and parent directory entries
        if (strcmp(child->d_name, ".") == 0 || strcmp(child->d_name, "..") == 0) {
            continue;
        }

        // Append child name to directory path
        sprintf(childPath, "%s/%s", directoryPath, child->d_name);

        // Handle child type
        if (child->d_type == DT_REG) {
            // Decrypt file
            decryptFile(childPath, key);
        } else if (child->d_type == DT_DIR) {
            // Decrypt directory
            decryptDirectory(childPath, key);
        }
    }

    // Close directory
    closedir(dp);
}

void encryptAny(char *location, uint8_t *key) {
    // Handle location type
    if (isFile(location)) {
        // Encrypt file
        encryptFile(location, key);
    } else if (isDirectory(location)) {
        // Encrypt directory
        encryptDirectory(location, key);
    }
}

void decryptAny(char *location, uint8_t *key) {
    // Handle location type
    if (isFile(location)) {
        // Decrypt file
        decryptFile(location, key);
    } else if (isDirectory(location)) {
        // Decrypt directory
        decryptDirectory(location, key);
    }
}

/*
/  argv[0]: executable
/  argv[1]: encrypt/decrypt
/  argv[2]: key
/  argv[3]..argv[argc-1]: file
*/
int main(int argc, char *argv[]) {
    // Make sure at least 4 arguments passed
    if (argc < 4) {
        // Print arguments error
        printf("ERROR: Invalid number of arguments!\n");

        // Exit program with error
        exit(EXIT_FAILURE);
    }
    
    // Handle encrypt/decrypt options
    if (strcmp(argv[1], "encrypt") == 0) {
        // Loop over arguments 3 to argc-1
        for (int i = 3; i < argc; i++) {
            // Encrypt location
            encryptAny(argv[i], argv[2]);

            // Print encrypted message
            printf("Encrypted %s!\n", argv[i]);
        }

    } else if (strcmp(argv[1], "decrypt") == 0) {
        // Loop over arguments 3 to argc-1
        for (int i = 3; i < argc; i++) {
            // Decrypt location
            decryptAny(argv[i], argv[2]);

            // Print decrypted message
            printf("Decrypted %s!\n", argv[i]);
        }
    }

    // Successfuly exit
    return EXIT_SUCCESS;
}
