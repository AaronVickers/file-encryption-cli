// Required headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "encrypt.h"

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
            // Encrypt location and store result
            Result *result = encryptAny(argv[i], argv[2]);

            // Check number of fails
            if (result->fails == 0) {
                // Check number of attempts
                if (result->attempts == 1) {
                    // Print file encrypted
                    printf("Encrypted %s!\n", argv[i]);
                } else if (result->attempts > 1) {
                    // Print directory encrypted with count
                    printf("Encrypted %d file(s) in %s!\n", result->attempts, argv[i]);
                } else {
                    // Print no files found
                    printf("No files found to encrypt in %s.\n", argv[i]);
                }
            } else {
                // Print number of fails
                printf("Failed to encrypt %d/%d file(s) in %s.\n", result->fails, result->attempts, argv[i]);
            }

            // Free result
            free(result);
        }

    } else if (strcmp(argv[1], "decrypt") == 0) {
        // Loop over arguments 3 to argc-1
        for (int i = 3; i < argc; i++) {
            // Decrypt location and store result
            Result *result = decryptAny(argv[i], argv[2]);

            // Check number of fails
            if (result->fails == 0) {
                // Check number of attempts
                if (result->attempts == 1) {
                    // Print file decrypted
                    printf("Decrypted %s!\n", argv[i]);
                } else if (result->attempts > 1) {
                    // Print directory decrypted with count
                    printf("Decrypted %d file(s) in %s!\n", result->attempts, argv[i]);
                } else {
                    // Print no files found
                    printf("No files found to decrypt in %s.\n", argv[i]);
                }
            } else {
                // Print number of fails
                printf("Failed to decrypt %d/%d file(s) in %s.\n", result->fails, result->attempts, argv[i]);
            }

            // Free result
            free(result);
        }
    }

    // Successfuly exit
    return EXIT_SUCCESS;
}
