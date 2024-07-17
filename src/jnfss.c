#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "jnfss.h"

#define SECTOR_SIZE 512
#define DEFAULT_ROOT_DIR_ENTRIES 224

#define UNKNOWN_STORAGE_MEDIA 0xF6
#define FLOPPY_DISK 0xF7
#define FIXED_DISK 0xF8

typedef struct {
    uint8_t JumpInst[3];
    uint16_t rootDIREntries;
    uint16_t totalSectors;
    uint8_t mediaType;
    uint8_t systemID[8];
    uint8_t volumeLabel[11];
}__attribute__((packed)) bootsector_t;

typedef struct {
    uint8_t filetype[3];
    uint8_t name[10];
    uint8_t allocated;
    uint8_t timeCreated[3];
    uint8_t timeModified[3];
    uint32_t dateCreated;
    uint32_t dateModified;
    uint16_t firstBlock;
    uint16_t size;
}__attribute__((packed)) record_t;

typedef struct {
    uint8_t data[509];
    uint8_t allocated;
    uint16_t nextBlock;
}__attribute__((packed)) diskblock_t;

bootsector_t* bootsect;
record_t* records;
diskblock_t* diskblock;

int sectorsPerRecord = 0;
int totalDiskBlocks = 0;
bool mounted = false;

uint8_t systemID[] = {0x4A, 0x4E, 0x46, 0x53, 0x53, 0x20, 0x20, 0x20};

bool createFS(const char* filepath, const char* volLabel) {
    if (mounted == true) return false;

    if (strlen(volLabel) > 11) {
        printf("ERROR: Volume Label Overflow [%s]\n\n", volLabel);
        return false;
    }

    FILE* disk = fopen(filepath, "rb");
    if (!disk) {
        printf("ERROR: Unable to open disk [%s]\n\n", filepath);
        return false;
    }

    bootsect = (bootsector_t*)malloc(sizeof(bootsector_t));
    memset(bootsect->JumpInst, 0x00, 3);
    bootsect->rootDIREntries = DEFAULT_ROOT_DIR_ENTRIES;

    fseek(disk, 0, SEEK_END);
    long size = ftell(disk);
    rewind(disk);
    bootsect->totalSectors = size / SECTOR_SIZE;

    bootsect->mediaType = UNKNOWN_STORAGE_MEDIA;
    memcpy(bootsect->systemID, systemID, sizeof(systemID));

    for (int i = 0; i < 11; i++) 
        bootsect->volumeLabel[i] = (uint8_t)volLabel[i];

    records = (record_t*)malloc(bootsect->rootDIREntries * sizeof(record_t));
    for (int i = 0; i < bootsect->rootDIREntries; i++)
        memset(&records[i], 0x00, sizeof(record_t));

    sectorsPerRecord = (bootsect->rootDIREntries * SECTOR_SIZE) / 16;
    totalDiskBlocks = bootsect->totalSectors - ((bootsect->rootDIREntries / 16) + 1);

    diskblock = (diskblock_t*)malloc(sizeof(diskblock_t) * totalDiskBlocks);
    for (int i = 0; i < totalDiskBlocks; i++)
        memset(&diskblock[i], 0x00, sizeof(diskblock_t));

    fclose(disk);
    
    mounted = true;
    return true;
}

bool mountFS(const char* filepath) {
    return false;                                   // WIP
}

bool syncFS(const char* filepath) {
    if (mounted == false) return false;

    FILE* disk = fopen(filepath, "wb+");
    if (!disk) {
        printf("ERROR: Unable to open disk [%s]\n\n", filepath);
        return false;
    }

    fseek(disk, 0, SEEK_SET);
    fwrite(bootsect, sizeof(bootsector_t), 1, disk);
    fseek(disk, SECTOR_SIZE, SEEK_SET);
    for (int i = 0; i < bootsect->rootDIREntries; i++)
        fwrite(&records[i], sizeof(record_t), 1, disk);

    fseek(disk, sectorsPerRecord + SECTOR_SIZE, SEEK_SET);
    for (int i = 0; i < totalDiskBlocks; i++)
        fwrite(&diskblock[i], sizeof(diskblock_t), 1, disk);

    free(bootsect);
    free(records);
    free(diskblock);

    fclose(disk);

    mounted = false;
    return true;
}

void printFSInfo() {
    if (mounted == false) return;

    printf("[JNFS Small]:\n");
    printf("Root Directory Entries: %d\n", bootsect->rootDIREntries);
    printf("Total Sectors: %d [%d KB]\n", bootsect->totalSectors, (bootsect->totalSectors * SECTOR_SIZE) / 1024);

    printf("Media Type: ");
    switch (bootsect->mediaType) {
        case UNKNOWN_STORAGE_MEDIA:
        printf("UNKNOWN STORAGE MEDIA\n");
        break;

        case FLOPPY_DISK:
        printf("FLOPPY DISK\n");
        break;

        case FIXED_DISK:
        printf("FIXED DISK\n");
        break;
    }

    printf("Volume Label: ");
    for(int i = 0; i < 11; i++)
        printf("%c", (char)bootsect->volumeLabel[i]);

    int freeBytes;
    int usedBytes;
    for (int i = 0; i < totalDiskBlocks; i++) {
        if (diskblock[i].allocated == 0x00)
            freeBytes += SECTOR_SIZE;
        if (diskblock[i].allocated == 0xFE)
            usedBytes += SECTOR_SIZE;
        freeBytes = i;
    }

    printf("\nFree Bytes: %d Bytes [%d KB]\n", freeBytes, freeBytes / 1024);
    printf("Used Bytes: %d Bytes [%d KB]\n\n", usedBytes, usedBytes / 1024);

    char* name = (char*)malloc(10 * sizeof(char));
    char* type = (char*)malloc(3 * sizeof(char));

    printf("[Record Info]: \n");
    for (int i = 0; i < bootsect->rootDIREntries; i++) {
        for (int j = 0; j < 10; j++)
            name[j] = records[i].name[j];
        name[11 - 1] = '\0';
        for (int j = 0; j < 3; j++)
            type[j] = records[i].filetype[j];
        type[4 - 1] = '\0';
        if (records[i].allocated == 0xFE) {
            printf("Record No.: %d\n", i);
            printf("Name: %s\n", name);
            printf("Filetype: %s\n", type);

            printf("Date Created: %d/%d/%d - %d:%d:%d\n", (uint8_t)((records[i].dateCreated >> 5) & 0x0F), (uint8_t)(records[i].dateCreated & 0x1F), (uint16_t)((records[i].dateCreated >> 8) & 0xFFFF), records[i].timeCreated[0], records[i].timeCreated[1], records[i].timeCreated[2]);
            printf("Date Modified: %d/%d/%d - %d:%d:%d\n", (uint8_t)((records[i].dateModified >> 5) & 0x0F), (uint8_t)(records[i].dateModified & 0x1F), (uint16_t)((records[i].dateModified >> 8) & 0xFFFF), records[i].timeModified[0], records[i].timeModified[1], records[i].timeModified[2]);

            printf("First Block No.: %d\n", records[i].firstBlock);

            printf("Size: %d Bytes\n\n", records[i].size);
        }
    }
}

int findEmptyRecord() {
    for (int i = 0; i < bootsect->rootDIREntries; i++)
        if (records[i].allocated == 0)
            return i;
    return -1;
}

int findEmptyBlock() {
    for (int i = 0; i < totalDiskBlocks; i++)
        if (diskblock[i].allocated == 0)
            return i;
    return -1;
}

int makeRecord(const char* name, const char* filetype) {
    if (mounted == false) return -1;

    if (strlen(name) > 10) {
        printf("ERROR: Filename Overflow [%s]\n\n", name);
        return -2;
    } else if (strlen(filetype) > 3) {
        printf("ERROR: Filetype Overflow [%s]\n\n", filetype);
        return -3;
    }

    int emptyRecord = findEmptyRecord();
    if (emptyRecord == -1) {
        printf("ERROR: Ran out of Records [Max: %d]\n\n", bootsect->rootDIREntries);
        return -4;
    }
    records[emptyRecord].allocated = 0xFE;
    
    for (int i = 0; i < 3; i++)
        records[emptyRecord].filetype[i] = (uint8_t)filetype[i];
    for (int i = 0; i < 10; i++)
        records[emptyRecord].name[i] = (uint8_t)name[i];

    time_t rawtime;
    struct tm* tinfo;

    time(&rawtime);
    tinfo = localtime(&rawtime);

    records[emptyRecord].timeCreated[0] = (uint8_t)tinfo->tm_hour;
    records[emptyRecord].timeCreated[1] = (uint8_t)tinfo->tm_min;
    records[emptyRecord].timeCreated[2] = (uint8_t)tinfo->tm_sec;
    for (int i = 0; i < 3; i++)
        records[emptyRecord].timeModified[i] = records[emptyRecord].timeCreated[i];

    records[emptyRecord].dateCreated |= ((uint32_t)tinfo->tm_mday & 0x1F);
    records[emptyRecord].dateCreated |= ((uint32_t)(tinfo->tm_mon + 1) & 0x0F) << 5;
    records[emptyRecord].dateCreated |= ((uint32_t)(tinfo->tm_year + 1900) & 0xFFFF) << 8;
    records[emptyRecord].dateModified = records[emptyRecord].dateCreated;

    int emptyBlock = findEmptyBlock();
    if (emptyBlock == -1) {
        printf("ERROR: Ran out of Blocks [Max: %d]\n\n", bootsect->totalSectors);
        return -5;
    }
    diskblock[emptyBlock].allocated = 0xFE;

    records[emptyRecord].firstBlock = emptyBlock;
    records[emptyRecord].size = SECTOR_SIZE;

    return emptyRecord;
}

int findRecord(const char* filename) {
    if (strlen(filename) > 10) return -2;

    int recordNum = -1;
    char* name = (char*)malloc(10 * sizeof(char));
    for (int i = 0; i < bootsect->rootDIREntries; i++) {
        memset(name, 0x00, 10);
        for (int j = 0; j < 10; j++)
            name[j] = records[i].name[j];
        name[11 - 1] = '\0';
        if (strcmp(filename, name) == 0) { recordNum = i; break; }
    }
    free(name);

    if (recordNum == -1 || records[recordNum].allocated == 0x00) return -3;

    return recordNum;
}

int removeRecord(const char* filename) {
    if (mounted == false) return -1;

    int recordNum = findRecord(filename);
    if (recordNum == -2 || recordNum == -3) return recordNum;
    int count = records[recordNum].size / SECTOR_SIZE;

    bool firstBlock = true;
    int block;
    while (true) {
        if (firstBlock) {
            block = records[recordNum].firstBlock;
            firstBlock = false;
        } else block = diskblock[block].nextBlock;

        diskblock[block].allocated = 0x00;
        count--;

        if (count == 0) break;
    }

    records[recordNum].allocated = 0x00;

    return 0;
}

int extendRecord(const char* filename, uint16_t count) {
    if (mounted == false || count == 0) return -1;

    int recordNum = findRecord(filename);
    if (recordNum == -2 || recordNum == -3) return recordNum;

    int recordAddSize = count * SECTOR_SIZE;

    bool firstBlock = true;
    int block;
    while (true) {
        if (firstBlock) {
            block = records[recordNum].firstBlock;
            firstBlock = false;
        } else block = diskblock[block].nextBlock;

        if (diskblock[block].nextBlock == 0) {
            int newBlock = findEmptyBlock();
            if (newBlock == -1) {
                printf("ERROR: Ran out of Blocks [Max: %d]\n\n", bootsect->totalSectors);
                return -3;
            }

            diskblock[block].nextBlock = newBlock;
            diskblock[newBlock].allocated = 0xFE;
            count--;
        }

        if (count == 0) break;
    }
    records[recordNum].size += recordAddSize;

    return 0;
}

int* readData(const char* filename, uint16_t size, uint16_t startBlock, uint16_t startByte) {
    if (mounted == false) return (int*)-1;
    
    int recordNum = findRecord(filename);
    if (recordNum < 0) {
        int* returnErr = &recordNum;
        return returnErr;
    }

    uint8_t* buffer = (uint8_t*)malloc(((size - startByte) * 2) * sizeof(uint8_t));  
    if (!buffer) return (int*)-4;

    int count = size / 509;

    int block;
    int lastPos = 0;
    bool startFinish = false;
    while (true) {
        if (startBlock == 0 && !startFinish) {
            block = records[recordNum].firstBlock;
            startFinish = true;
        } else block = diskblock[block].nextBlock;

        int start = (startByte != 0) ? startByte : 0;
        startByte = 0;

        if (diskblock[block].allocated == 0xFE) {
            for (int i = start; i < 509; i++) {
                buffer[lastPos] = diskblock[block].data[i];
                lastPos++;
            }
        }

        if (count == 0) break;
        count--;
    }

    return (int*)buffer;
}

int fillData(const char* filename, void* buffer, uint16_t size, uint16_t startBlock, uint16_t startByte) {
    if (mounted == false) return -1;
    
    int recordNum = findRecord(filename);
    if (recordNum == -2 || recordNum == -3) return recordNum;

    uint8_t* u8Buffer = (uint8_t*)malloc(size * sizeof(uint8_t));
    if (!buffer) return -4;
    memset(u8Buffer, 0x00, size);
    memcpy(u8Buffer, buffer, size);

    int count = size / 509;

    int block;
    int lastPos = 0;
    bool startFinish = false;
    while (true) {
        if (startBlock == 0 && !startFinish) {
            block = records[recordNum].firstBlock;
            startFinish = true;
        } else block = diskblock[block].nextBlock;

        int start = (startByte != 0) ? startByte : 0;
        startByte = 0;

        if (diskblock[block].allocated == 0xFE) {
            for (int i = start; i < 509; i++) {
                diskblock[block].data[i] = u8Buffer[lastPos];
                lastPos++;
            }
        }

        if (count == 0) break;
        count--;
    }

    return 0;
}

int renameFile(const char* oldFilename, const char* newFilename) {
    if (strlen(newFilename) > 10) return -1;

    int recordNum = findRecord(oldFilename);
    if (recordNum == -2 || recordNum == -3) return recordNum;

    for (int i = 0; i < 10; i++)
        records[recordNum].name[i] = (uint8_t)newFilename[i];

    return 0;
}