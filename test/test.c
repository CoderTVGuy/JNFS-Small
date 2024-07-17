#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/jnfss.h"

bool mount;
void getFile(const char* filename, long filesize) {
    FILE* outFile = fopen(filename, "wb+");
    
    uint8_t* buffer = (uint8_t*)malloc(filesize * sizeof(uint8_t));
    if (!buffer) {
        printf("Memory Allocation Failed\n");
        exit(EXIT_FAILURE);
    }

    buffer = (uint8_t*)readData("WINXP     ", filesize, 0, 0);
    fwrite(buffer, filesize, 1, outFile);

    fclose(outFile);
    free(buffer);
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        printf("Usage: %s <filename> <operation> <diskname>\n\n", argv[0]);
        return 1;
    }

    FILE* file = fopen(argv[1], "rb");
    if (!file) {
        printf("File not found: [%s]\n", argv[1]);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);

    if (filesize == -1) {
        printf("File size error: [%s]\n", argv[1]);
        return 1;
    }

    rewind(file);

    if (strcmp(argv[2], "create") == 0) {
        char* volName = (char*)malloc(11 * sizeof(char));
        memset(volName, 0x00, 11);
        printf("Volume Label: ");
        scanf("%s", volName);

        createFS(argv[3], volName);
        makeRecord("WINXP     ", "JPG");
        extendRecord("WINXP     ", filesize / 509);

        free(volName);
    } else {
        printf("Unknown operation [%s]\n", argv[2]);
        return 1;
    }

    if (mount == true) getFile("hello.exe", filesize);

    uint8_t* fileData = (uint8_t*)malloc(filesize * sizeof(uint8_t));
    if (!fileData) {
        printf("Memory Allocation Failed\n");
        return 1;
    }

    fread(fileData, filesize, 1, file);

    fillData("WINXP     ", (void*)fileData, filesize, 0, 0);

    fclose(file);
    free(fileData);

    getFile("hello.exe", filesize);

    printFSInfo();

    syncFS(argv[3]);

    return 0;
}