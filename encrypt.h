// Required headers
#include <stdint.h>

typedef struct {
    int attempts;
    int fails;
} Result;

uint8_t *generateIV(int length);

int encryptFile(char *filePath, uint8_t *key);
int decryptFile(char *filePath, uint8_t *key);

Result *encryptDirectory(char *directoryPath, uint8_t *key);
Result *decryptDirectory(char *directoryPath, uint8_t *key);

Result *encryptAny(char *location, uint8_t *key);
Result *decryptAny(char *location, uint8_t *key);