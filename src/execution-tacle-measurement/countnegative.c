/*

  This program is part of the TACLeBench benchmark suite.
  Version V 2.0

  Name: countnegative

  Author: unknown

  Function: Counts negative and non-negative numbers in a
    matrix. Features nested loops, well-structured code.

  Source: MRTC
          http://www.mrtc.mdh.se/projects/wcet/wcet_bench/cnt/cnt.c

  Changes: Changed split between initialization and computation

  License: May be used, modified, and re-distributed freely

*/

#include "inc/TACle.h"

/*
  Type definition for the matrix
*/
typedef int matrix [ CN_MAXSIZE ][ CN_MAXSIZE ];

/*
  Forward declaration of functions
*/
void countnegative_initSeed( void );
int countnegative_randomInteger( void );
void countnegative_initialize( matrix );
int countnegative_sum( matrix );

/*
  Globals
*/
volatile int countnegative_seed;
matrix countnegative_array;

/*
  Initializes the seed used in the random number generator.
*/
void countnegative_initSeed ( void )
{
  countnegative_seed = 0;
}

/*
  Generates random integers between 0 and 8094
*/
int countnegative_randomInteger( void )
{
  countnegative_seed = ( ( countnegative_seed * 133 ) + 81 ) % 8095;
  return  countnegative_seed;
}

/*
  Initializes the given array with random integers.
*/
void countnegative_initialize( matrix Array )
{
  register int OuterIndex, InnerIndex;

  _Pragma( "loopbound min 20 max 20" )
  for ( OuterIndex = 0; OuterIndex < CN_MAXSIZE; OuterIndex++ )
    _Pragma( "loopbound min 20 max 20" )
    for ( InnerIndex = 0; InnerIndex < CN_MAXSIZE; InnerIndex++ )
      Array[ OuterIndex ][ InnerIndex ] =  countnegative_randomInteger();
}

void countnegative_init( void )
{
  countnegative_initSeed();
  countnegative_initialize( countnegative_array );
}

int countnegative_sum( matrix Array )
{
  register int Outer, Inner;

  register int Ptotal = 0; 
  register int Ntotal = 0;
  register int Pcnt = 0;
  register int Ncnt = 0;

  _Pragma( "loopbound min 20 max 20" )
  for ( Outer = 0; Outer < CN_MAXSIZE; Outer++ )
    _Pragma( "loopbound min 20 max 20" )
    for ( Inner = 0; Inner < CN_MAXSIZE; Inner++ )
      if ( Array[ Outer ][ Inner ] >= 0 ) {
        Ptotal += Array[ Outer ][ Inner ];
        Pcnt++;
      } else {
        Ntotal += Array[ Outer ][ Inner ];
        Ncnt++;
      }

  return Ptotal + Pcnt + Ntotal + Ncnt;
}

unsigned char *get_countnegative_array(void) 
{
  return (unsigned char *)countnegative_array;
}

/*
  The main function
*/
void countnegative_main ( void )
{
  countnegative_sum(  countnegative_array );
}

// int main( void )
// {
//   countnegative_init();
//   countnegative_main();

//   return ( countnegative_return() );
// }
