#ifndef __DATA_H__
#define __DATA_H__

// Defines for sizes
#define kB *1024
#define MB kB*1024
#define GB MB*1024

/* Kilobyte from byte value */
#define BtkB(size) (size / 1024)

// Application data
#define MAX_DATA_SIZE 512 kB

#include <appdata.h>

#ifndef __APPDATA_H__
uint8_t appdata[MAX_DATA_SIZE] = {[0 ... MAX_DATA_SIZE - 1] = 1};
#endif

#endif