#ifndef JNFS_SMALL_H
#define JNFS_SMALL_H

#include <stdint.h>
#include <stdbool.h>

bool createFS(const char* filepath, const char* volLabel);
bool mountFS(const char* filepath);
bool syncFS(const char* filepath);

int makeRecord(const char* name, const char* filetype);
int removeRecord(const char* filename);
int extendRecord(const char* filename, uint16_t count);
int fillData(const char* filename, void* buffer, uint16_t size, uint16_t startBlock, uint16_t startByte);
int* readData(const char* filename, uint16_t size, uint16_t startBlock, uint16_t startByte);
int renameFile(const char* oldFilename, const char* newFilename);
int findRecord(const char* filename);

void printFSInfo();

#endif