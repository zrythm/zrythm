/*
  ==============================================================================
  
  This file is part of the dRowAudio JUCE module
  Copyright 2004-12 by dRowAudio.
  
  ------------------------------------------------------------------------------
 
  dRowAudio can be redistributed and/or modified under the terms of the GNU General
  Public License (Version 2), as published by the Free Software Foundation.
  A copy of the license is included in the module distribution, or can be found
  online at www.gnu.org/licenses.
  
  dRowAudio is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
  
  ==============================================================================
 */

#ifndef __DROWAUDIO_CUMULATIVEMOVINGAVERAGE_H__
#define __DROWAUDIO_CUMULATIVEMOVINGAVERAGE_H__

//==============================================================================
/**
    Simple cumulative average class which you can add values to and will return
    the mean of them. This can be used when you don't know the total number of
    values that need to be averaged.
 */
class CumulativeMovingAverage
{
public:
    //==============================================================================
    /** Creates a blank CumulativeMovingAverage.
     */
    CumulativeMovingAverage() noexcept
        : currentAverage (0.0),
          numValues (0)
    {
    }
    
    /** Creates a copy of another CumulativeMovingAverage.
     */
    CumulativeMovingAverage (const CumulativeMovingAverage& other) noexcept
    {
        currentAverage = other.currentAverage;
        numValues = other.numValues;
    }
    
    /** Destructor.
     */
    ~CumulativeMovingAverage()
    {
    }
    
    /** Resets the CumulativeMovingAverage.
     */
    void reset() noexcept
    {
        currentAverage = 0.0;
        numValues = 0;
    }
    
    /** Adds a new value to contribute to the average and returns the average.
     */
    inline double add (double newValue) noexcept
    {
        currentAverage = (newValue + (numValues * currentAverage)) / (numValues + 1);
        ++numValues;
        
        return currentAverage;
    }
    
    /** Returns the current average.
     */
    inline double getAverage()                      {   return currentAverage;  }
    
    /** Returns the number of values that have contributed to the current average.
     */
    inline int getNumValues()                       {   return numValues;       }
    
private:
    //==============================================================================
    double currentAverage;
    int numValues;
    
    JUCE_LEAK_DETECTOR (CumulativeMovingAverage);
};


#endif //__DROWAUDIO_CUMULATIVEMOVINGAVERAGE_H__