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

// Bubblesort
// Data
/*
  The dimension of the matrix
*/
#define BS_MAXSIZE 204800 // 200kB

// Functions
unsigned char *get_bubblesort_array(void);
void bubblesort_init(void);
void bubblesort_main(void);

#endif