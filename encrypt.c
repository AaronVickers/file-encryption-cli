// Include header file
#include "encrypt.h"

// Required headers
#include <dirent.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "utils.h"

// Include AES header
#include "aes.h"

// Initialization vector size
#define IV_SIZE 16

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

int encryptFile(char *filePath, uint8_t *key) {
    // Open unencrypted file in read mode
    FILE *fp = fopen(filePath, "r");

    // Get file size
    long fileSize = getFileSize(fp);

    // Get key length
    size_t keyLength = strlen(key);

    // Check if file size invalid
    if (fileSize < 1) {
        // Return failed
        return 1;
    }

    // Calculate file buffer size in bytes
    long fileBufferSize = fileSize + keyLength;

    // Allocate IV and file content buffers
    uint8_t *fileIV = generateIV(IV_SIZE);
    char *fileBuffer = (char*) malloc(sizeof(char) * fileBufferSize);

    // Read file content into buffer and close file
    fread(fileBuffer, 1, fileSize, fp);
    fclose(fp);

    // Put key at end of file buffer
    memcpy(fileBuffer + fileSize, key, keyLength);

    // Encrypt buffer using AES-CTR, IV, and key
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, fileIV);
    AES_CTR_xcrypt_buffer(&ctx, fileBuffer, fileBufferSize);

    // Open unencrypted file in write mode
    fp = fopen(filePath, "w");

    // Write IV and encrypted content and close file
    fwrite(fileIV, 1, IV_SIZE, fp);
    fwrite(fileBuffer, 1, fileBufferSize, fp);
    fclose(fp);

    // Free allocated buffers
    free(fileIV);
    free(fileBuffer);

    // Return success
    return 0;
}

int decryptFile(char *filePath, uint8_t *key) {
    // Open encrypted file in read mode
    FILE *fp = fopen(filePath, "r");

    // Get file size
    long fileSize = getFileSize(fp);

    // Get key length
    size_t keyLength = strlen(key);

    // Check if file size invalid
    if (fileSize <= IV_SIZE) {
        // Return failed
        return 1;
    }

    // Calculate file buffer size in bytes
    long fileBufferSize = fileSize - IV_SIZE;

    // Calculate file content size in bytes
    long fileContentSize = fileBufferSize - keyLength;

    // Allocate IV and file content buffers
    uint8_t *fileIV = (uint8_t*) malloc(sizeof(uint8_t) * IV_SIZE);
    char *fileBuffer = (char*) malloc(sizeof(char) * fileBufferSize);

    // Read IV and file content into buffers and close file
    fread(fileIV, 1, IV_SIZE, fp);
    fread(fileBuffer, 1, fileBufferSize, fp);
    fclose(fp);

    // Decrypt buffer using AES-CTR, IV, and key
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, fileIV);
    AES_CTR_xcrypt_buffer(&ctx, fileBuffer, fileBufferSize);

    // Validate file integrity
    if (strncmp(fileBuffer + fileContentSize, key, keyLength) != 0) {
        // Return failed
        return 1;
    }

    // Open encrypted file in write mode
    fp = fopen(filePath, "w");

    // Write unencrypted content and close file
    fwrite(fileBuffer, 1, fileContentSize, fp);
    fclose(fp);

    // Free allocated buffers
    free(fileIV);
    free(fileBuffer);

    // Return success
    return 0;
}

Result *encryptDirectory(char *directoryPath, uint8_t *key) {
    // Result store
    Result *result = malloc(sizeof(Result));

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
            // Increase number of attempts
            result->attempts += 1;

            // Encrypt file and increase fail count if failed
            result->fails += encryptFile(childPath, key);
        } else if (child->d_type == DT_DIR) {
            // Encrypt directory and store result
            Result *childResult = encryptDirectory(childPath, key);

            // Add child result to result
            result->attempts += childResult->attempts;
            result->fails += childResult->fails;

            // Free child result
            free(childResult);
        }
    }

    // Close directory
    closedir(dp);

    // Return result
    return result;
}

Result *decryptDirectory(char *directoryPath, uint8_t *key) {
    // Result store
    Result *result = malloc(sizeof(Result));

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
            // Increase number of attempts
            result->attempts += 1;

            // Decrypt file and increase fail count if failed
            result->fails += decryptFile(childPath, key);
        } else if (child->d_type == DT_DIR) {
            // Decrypt directory and store result
            Result *childResult = decryptDirectory(childPath, key);

            // Add child result to result
            result->attempts += childResult->attempts;
            result->fails += childResult->fails;

            // Free child result
            free(childResult);
        }
    }

    // Close directory
    closedir(dp);

    // Return result
    return result;
}

Result *encryptAny(char *location, uint8_t *key) {
    // Handle location type
    if (isFile(location)) {
        // Result store
        Result *result = malloc(sizeof(Result));

        // Set attempts to 1
        result->attempts = 1;

        // Encrypt file and set fail count
        result->fails += encryptFile(location, key);

        // Return result
        return result;
    } else if (isDirectory(location)) {
        // Encrypt directory and return result
        return encryptDirectory(location, key);
    }
}

Result *decryptAny(char *location, uint8_t *key) {
    // Handle location type
    if (isFile(location)) {
        // Result store
        Result *result = malloc(sizeof(Result));

        // Set attempts to 1
        result->attempts = 1;

        // Decrypt file and set fail count
        result->fails += decryptFile(location, key);

        // Return result
        return result;
    } else if (isDirectory(location)) {
        // Decrypt directory and return result
        return decryptDirectory(location, key);
    }
}
