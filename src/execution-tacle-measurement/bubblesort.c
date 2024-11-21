/*
 * At first I wanted to write a bubble sort algorithm since it takes a long time to 
 * execute. The problem is that it writes to the array and will create interferences...
 * I thus decided to keep the bubble sort spirit and to count, for each integer, the 
 * number of bigger numbers in the array. It will give an upper bound of the number
 * of swaps to make. 
 */

#include "inc/TACle.h"

/*
    Forward declaration of functions
*/

void bubblesort_initSeed(void);
long bubblesort_randomInteger(void);
long int bubblesort_sort(void);

/*
    Globals
*/
unsigned long long int bubblesort_array[BS_MAXSIZE];
volatile int bubblesort_seed;

/*
    bubblesort_initSeed initializes the seed used in the "random" number
    generator.
*/
void bubblesort_initSeed(void)
{
    bubblesort_seed = 0;
}

/*
    bubblesort_RandomInteger generates "random" integers between 0 and 8094.
*/
long bubblesort_randomInteger(void)
{
    bubblesort_seed = ((bubblesort_seed * 133) + 81) % 8095;
    return (bubblesort_seed);
}

/*
    Sorting function
*/
long int bubblesort_sort(void)
{
    // Writes the result so not possible to avoid executing
    volatile long int result = 0;
    register long int possible_swap = 0;

    for (int i = 0; i < BS_MAXSIZE - 1; i++)
    {
        for (int j = 0; j < BS_MAXSIZE; j++)
        {
            // printf("%d\n", bubblesort_array[j] );
            // If a bigger number exists, then add one
            if (bubblesort_array[j] > bubblesort_array[i])
            {
                possible_swap += 1;
            }
        }
    }

    result = possible_swap;
    return result;
}

/*
    Init function
*/
void bubblesort_init(void)
{
    int i;
    bubblesort_initSeed();

    for (i = 0; i < BS_MAXSIZE; ++i)
    {
        bubblesort_array[i] = (bubblesort_randomInteger() & 0xFF0) >> 4;
    }
}

unsigned char *get_bubblesort_array(void)
{
    return (unsigned char *)bubblesort_array;
}

void bubblesort_main(void *arg)
{
    bubblesort_sort();
}