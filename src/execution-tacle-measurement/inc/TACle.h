#ifndef __TACLE_H__
#define __TACLE_H__

// MPEG2
// Functions
unsigned char *get_mpeg2_oldorgframe(void);
void mpeg2_main(void);

// Countnegative
// Data
/*
  The dimension of the matrix
*/
#define CN_MAXSIZE 96 // 96 int = 384B, 384 x 384 = 147456B = 144kB

// Functions
unsigned char *get_countnegative_array(void);
void countnegative_init(void);
void countnegative_main(void);

// Binarysearch
// Data
/*
  The dimension of the matrix
*/
#define BN_MAXSIZE 25600 // 25600 * 2 int = 51200 * 4 B = 4 * 50kB = 200kB

// Functions
unsigned char *get_binarysearch_array(void);
void binarysearch_init(void);
void binarysearch_main(void);

#endif