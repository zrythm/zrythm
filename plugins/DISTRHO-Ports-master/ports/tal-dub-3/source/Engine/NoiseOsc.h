#ifndef NoiseOsc_H
#define NoiseOsc_H

#include "math.h"

/*-----------------------------------------------------------------------------

Kunz Patrick 30.04.2007

White noise oscillator.

-----------------------------------------------------------------------------*/

#include <cstdlib>

class NoiseOsc
{
  public:
  float randomValue;

  NoiseOsc(float sampleRate) 
  {
    resetOsc();
  }

  void resetOsc() 
  {
    randomValue= 0;
  }

  inline float getNextSample() 
  {
    //randomValue= (randomValue*2.0f+(float)rand()/RAND_MAX)*0.33333333333333333333f;
    //return (randomValue-0.5f)*2.0f;
    randomValue= ((float)rand()/RAND_MAX);
    return (randomValue-0.5f)*2.0f;
  }
};
#endif