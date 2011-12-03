/***************************** RANDOMC.H *********************** 2001-10-24 AF *
*
* This file contains class declarations for the C++ library of uniform
* random number generators.
*
* Overview of classes:
* ====================
*
* class TRandomMersenne:
* Random number generator of type Mersenne twister.
* Source file mersenne.cpp
*
* class TRandomMotherOfAll:
* Random number generator of type Mother-of-All (Multiply with carry).
* Source file mother.cpp
*
* class TRanrotBGenerator:
* Random number generator of type RANROT-B.
* Source file ranrotb.cpp
*
* class TRanrotWGenerator:
* Random number generator of type RANROT-W.
* Source file ranrotw.cpp
*
* class TRandomMotRot:
* Combination of Mother-of-All and RANROT-W generators.
* Source file ranmoro.cpp and motrot.asm.
* Coded in assembly language for improved speed.
* Must link in RANDOMAO.LIB or RANDOMAM.LIB.
*
*
* Member functions (methods):
* ===========================
*
* All these classes have identical member functions:
*
* Constructor(uint32 seed):
* The seed can be any integer. Usually the time is used as seed.
* Executing a program twice with the same seed will give the same sequence of
* random numbers. A different seed will give a different sequence.
*
* void RandomInit(uint32 seed);
* Re-initializes the random number generator with a new seed.
*
* void RandomInitByArray(uint32 seeds[], int length);
* In TRandomMersenne only: Use this function if you want to initialize with
* a seed with more than 32 bits. All bits in the seeds[] array will influence
* the sequence of random numbers generated. length is the number of entries
* in the seeds[] array.
*
* double Random();
* Gives a floating point random number in the interval 0 <= x < 1.
* The resolution is 32 bits in TRanrotBGenerator, TRandomMotherOfAll and
* TRandomMersenne. 52 or 63 bits in TRanrotWGenerator. 63 bits in 
* TRandomMotRot.
*
* int IRandom(int min, int max);
* Gives an integer random number in the interval min <= x <= max.
* (max-min < MAXINT).
* The resolution is the same as for Random(). 
*
* uint32 BRandom();
* Gives 32 random bits. 
* Only available in the classes TRanrotWGenerator and TRandomMersenne.
*
*
* Example:
* ========
* The file EX-RAN.CPP contains an example of how to generate random numbers.
*
*
* Further documentation:
* ======================
* The file randomc.htm contains further documentation on these random number
* generators.
*
* © 1997 - 2004 Agner Fog. 
* GNU General Public License www.gnu.org/copyleft/gpl.html
*******************************************************************************/

/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.1  2006/11/04 10:47:28  sparhawk
 * Advanced randomizer added.
 *
 *
 ***************************************************************************/

#ifndef RANDOMC_H
#define RANDOMC_H

#ifdef __INTEL_COMPILER
  #include <mathimf.h>       // Intel math function library
#else
  #include <math.h>          // default math function linrary
#endif

#include <assert.h>
#include <stdio.h>

// Define 32 bit signed and unsigned integers.
// Change these definitions, if necessary, on 64 bit computers
typedef   signed long int32;     
typedef unsigned long uint32;     

class TRandomMersenne {                // encapsulate random number generator
  #if 0
    // define constants for MT11213A:
    // (32 bit constants cannot be defined as enum in 16-bit compilers)
    #define MERS_N   351
    #define MERS_M   175
    #define MERS_R   19
    #define MERS_U   11
    #define MERS_S   7
    #define MERS_T   15
    #define MERS_L   17
    #define MERS_A   0xE4BD75F5
    #define MERS_B   0x655E5280
    #define MERS_C   0xFFD58000
  #else    
    // or constants for MT19937:
    #define MERS_N   624
    #define MERS_M   397
    #define MERS_R   31
    #define MERS_U   11
    #define MERS_S   7
    #define MERS_T   15
    #define MERS_L   18
    #define MERS_A   0x9908B0DF
    #define MERS_B   0x9D2C5680
    #define MERS_C   0xEFC60000
  #endif
  public:
  TRandomMersenne(uint32 seed) {       // constructor
    RandomInit(seed);}
  void RandomInit(uint32 seed);        // re-seed
  void RandomInitByArray(uint32 seeds[], int length); // seed by more than 32 bits
  int IRandom(int min, int max);       // output random integer
  double Random();                     // output random float
  uint32 BRandom();                    // output random bits
  private:
  uint32 mt[MERS_N];                   // state vector
  int mti;                             // index into mt
  enum TArch {LITTLE_ENDIAN1, BIG_ENDIAN1, NONIEEE};
  TArch Architecture;                  // conversion to float depends on computer architecture
  };    

class TRanrotBGenerator {              // encapsulate random number generator
  enum constants {                     // define parameters
    KK = 17, JJ = 10, R1 = 13, R2 =  9};
  public:
  void RandomInit(uint32 seed);        // initialization
  int IRandom(int min, int max);       // get integer random number in desired interval
  double Random();                     // get floating point random number
  TRanrotBGenerator(uint32 seed);      // constructor
  protected:
  int p1, p2;                          // indexes into buffer
  uint32 randbuffer[KK];               // history buffer
  uint32 randbufcopy[KK*2];            // used for self-test
  enum TArch {LITTLE_ENDIAN1, BIG_ENDIAN1, NONIEEE};
  TArch Architecture;                  // conversion to float depends on computer architecture
};


class TRanrotWGenerator {              // encapsulate random number generator
  enum constants {                     // define parameters
    KK = 17, JJ = 10, R1 = 19, R2 =  27};
  public:
  void RandomInit(uint32 seed);        // initialization
  int IRandom(int min, int max);       // get integer random number in desired interval
  long double Random();                // get floating point random number
  uint32 BRandom();                    // output random bits  
  TRanrotWGenerator(uint32 seed);      // constructor
  protected:
  int p1, p2;                          // indexes into buffer
  union {                              // used for conversion to float
    long double randp1;
    uint32 randbits[3];};  
  uint32 randbuffer[KK][2];            // history buffer
  uint32 randbufcopy[KK*2][2];         // used for self-test
  enum TArch {LITTLE_ENDIAN1, BIG_ENDIAN1, NONIEEE, EXTENDEDPRECISIONLITTLEENDIAN};
  TArch Architecture;                  // conversion to float depends on computer architecture
};

class TRandomMotherOfAll {             // encapsulate random number generator
  public:
  void RandomInit(uint32 seed);        // initialization
  int IRandom(int min, int max);       // get integer random number in desired interval
  double Random();                     // get floating point random number
  TRandomMotherOfAll(uint32 seed);     // constructor
  protected:
  double x[5];                         // history buffer
  };

#include "rancombi.h"

#endif

