// Temporary fix - defines "uxrandom" function
#include "rand.h"

UBaseType_t ulNextRand = 0;

UBaseType_t uxRand(void) {
  const uint32_t ulMultiplier = 0x015a4e35UL, ulIncrement = 1UL;

  /* Utility function to generate a pseudo random number. */
  ulNextRand = (ulMultiplier * ulNextRand) + ulIncrement;
  return ((int)(ulNextRand >> 16UL) & 0x7fffUL);
}

/*
 * Set *pulNumber to a random number, and return pdTRUE. When the random number
 * generator is broken, it shall return pdFALSE.
 * The macros ipconfigRAND32() and configRAND32() are not in use
 * anymore in FreeRTOS+TCP.
 *
 * THIS IS ONLY A DUMMY IMPLEMENTATION THAT RETURNS A PSEUDO RANDOM NUMBER SO IS
 * NOT INTENDED FOR USE IN PRODUCTION SYSTEMS.
 */
BaseType_t xApplicationGetRandomNumber( uint32_t * pulNumber )
{
    *pulNumber = uxRand();
    return pdTRUE;
}
