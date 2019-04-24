/*-----------------------------------------------------------------------------

DC Blocking Filter                      
by Perry R. Cook, 1995-96               
                                         
This guy is very helpful in, uh,       
blocking DC.  Needed because a simple
low-pass reflection filter allows DC to
to build up inside recursive structures
-----------------------------------------------------------------------------*/

#if !defined(__DCBlock_h)
#define __DCBlock_h

class DCBlock 
{
 public:
  float inputs, outputs, lastOutput;

  DCBlock() 
  {
    lastOutput = inputs = outputs = 0.0f;
  }

  inline void tick(float *sample, float cutoff) 
  {
    outputs     = *sample-inputs+(0.999f-cutoff*0.4f)*outputs;
    inputs      = *sample;
    lastOutput  = outputs;
    *sample     = lastOutput;
  }
};

#endif
