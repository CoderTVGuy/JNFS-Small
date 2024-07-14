#ifndef JNFS_SMALL_H
#define JNFS_SMALL_H

#include <stdint.h>
#include <stdbool.h>

bool createFS(const char* filepath, const char* volLabel);
bool mountFS(const char* filepath);
bool syncFS(const char* filepath);

int makeRecord(const char* name, const char* filetype);
int extendBlock(const char* filename, uint16_t count);

void printFSInfo();

#endif