/*
  ==============================================================================

  This file is part of the dRowAudio JUCE module
  Copyright 2004-13 by dRowAudio.

  ------------------------------------------------------------------------------

  dRowAudio is provided under the terms of The MIT License (MIT):

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
  SOFTWARE.

  ==============================================================================
*/

#ifndef __DROWAUDIO_STATEVARIABLE_H__
#define __DROWAUDIO_STATEVARIABLE_H__

//==============================================================================
/** Variable that holds its previous value.
 
	This can be used instead of keeping track of a current and previous state
	of a primitive variable. Just calling set() will automatically update the
	new and previous states.
 */
template <class VariableType>
class StateVariable
{
public:
    //==============================================================================
	/**	Create a StateVariable with an initial value of 0.
	 */
	StateVariable()
	{
		previous = current = 0;
	}
	
	/**	Create a StateVariable with an initial value.
		To begin with the previous value will be the same as the initial.
	 */
	StateVariable (VariableType initialValue)
	{
		previous = current = initialValue;
	}
	
	/** Destructor. */
	~StateVariable() {}
	
    /** Returns the current value.
     */
	inline VariableType getCurrent()	{	return current;		}

    /** Returns the previous value.
     */
    inline VariableType getPrevious()	{	return previous;	}
	
    /** Sets a new value, copying the current to the previous.
     */
	inline void set (VariableType newValue)
	{
		previous = current;
		current = newValue;
	}
	
    /** Sets the current value, leaving the previous unchanged.
     */
	inline void setOnlyCurrent (VariableType newValue)
	{
		current = newValue;
	}
	
    /** Sets the previous value, leaving the current unchanged.
     */
	inline void setPrevious(VariableType newValue)
	{
		previous = newValue;
	}

    /** Sets the current and the previous value to be the same.
     */
	inline void setBoth (VariableType newValue)
	{
		current = previous = newValue;
	}
	
	/**	Returns true if the current and previous states are equal.
	 */
	inline bool areEqual()
	{
		return (previous == current);
	}
	
    /** Returns true if the two are almost equal to a given precision.
     */
	inline bool areAlmostEqual (double precision = 0.00001)
	{
		return almostEqual (current, previous, precision);
	}

    /** The same as calling set().
     */
	inline void operator= (VariableType newValue)
	{
		previous = current;
		current = newValue;
	}

    /** Incriments the current value by newValue and updates the previous.
     */
	inline void operator+= (VariableType newValue)
	{
		previous = current;
		current += newValue;
	}

    /** Multiplies the current value by newValue and updates the previous.
     */
	inline void operator*= (VariableType newValue)
	{
		previous = current;
		current *= newValue;
	}
		
	/**	This returns the difference between the current and the previous state.
	 */
	inline VariableType getDifference()
	{
		return current - previous;
	}
	
private:
    //==============================================================================
	VariableType current, previous;
	
    //==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StateVariable);
};

#endif //__DROWAUDIO_STATEVARIABLE_H__