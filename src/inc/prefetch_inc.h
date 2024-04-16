#ifndef __PREFETCH_INC_H__
#define __PREFETCH_INC_H__

#if defined(PLATFORM) && PLATFORM == rpi4
    #define L2_CACHE_SIZE 0x100000
#else
    #define L2_CACHE_SIZE L2_BLOCK_SIZE
#endif

#define D_MIN_LINE_OFF  16
#define D_MIN_LINE_LEN  4
#define WORD_SIZE       4

#define L2_CACHE_LINE_SIZE      64
#define LOG2_L2_CACHE_LINE_SIZE 6

#endif