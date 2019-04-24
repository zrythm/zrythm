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

#ifndef __DROWAUDIO_BUFFER_H__
#define __DROWAUDIO_BUFFER_H__

//==============================================================================
/** A buffer to hold an array of floats.
 
	This is a simple container for an array of floats that can be used to store 
	common values such as a waveshape or look-up table.
	Create one of these on the stack for most efficient and safe use.
 
	You can attach listners that inherit from Buffer::Listener to a Buffer
	to be notified when they change their contents using Buffer::addListener.
	This can be useful in updating UI elements such as wave displays.
 */
class Buffer
{
public:
    //==============================================================================
	/** Creates an empty Buffer. */
	Buffer()
        : bufferSize (0)
    {
    }

	/** Creates a buffer with a given size. */
	Buffer (int size)
        : bufferSize (size)
    {
        buffer.allocate (bufferSize, true);
    }
	
	/** Creates a copy of another buffer. */
	Buffer (const Buffer& otherBuffer)
        : bufferSize (otherBuffer.bufferSize)
    {
        buffer.allocate (bufferSize, false);
        memcpy (buffer, otherBuffer.buffer, bufferSize * sizeof (float));
    }

	/**	Destructor. */
	~Buffer() {}
	
	/** Changes the size of the buffer.
		This will change the size of the buffer, keeping as much of the existing data
		as possible. Any elements beyond the orginal ones will have a value of zero.
		Therefore it is best to either reset the whole buffer or refill it from your own
		algorithm before using it.
	 */
	void setSize (int newSize)
    {
        buffer.realloc (newSize);
        
        if (newSize > bufferSize)
            zeromem (buffer + bufferSize, (newSize - bufferSize) * sizeof (float));
        
        bufferSize = newSize;
    }
	
	/**	Changes the size of the buffer.
		This does the same as setSize() but slightly quicker with the expense of possibly
		having rubbish in the buffer. Be sure to either refill or reset the buffer before using it.
	 */
	inline void setSizeQuick (int newSize)
    {
        buffer.malloc (newSize);
        bufferSize = newSize;
    }
	
	/** Returns a value from the buffer.
		This method performs no bounds checking so if the index is out of the internal array
		bounds will contain garbage.
	 */
	inline float operator[](const int index)        { return buffer[index]; }
	
	/** Returns a reference to a value from the buffer.
		This method returns a reference to an element from the buffer so can therefore be changed.
		This method also performs no bounds checking so if the index is out of the internal array
		bounds will contain garbage.
	 */
	inline float& getReference (const int index)    { return buffer[index]; }
	
	/** Zeros the buffer's contents.
	 */
	inline void reset()                             { zeromem (buffer, bufferSize * sizeof (float)); }
		
	/** Returns a pointer to the beggining of the data.
		Don't hang on to this pointer as it may change if the buffer is internally re-allocated.
	 */
	inline float* getData()                         { return buffer.getData(); }
	
	/** Returns the current size of the buffer. */
	inline int getSize() const                      { return bufferSize; }
	
	/**	Copies the contents of a section of memory into the internal buffer.
	 
		This is simpler than using the [] operator or getData and your own for loop if you are
		copying a buffer you have already created, especially if the sizes may be different.
		If resizeToFit is true the internal array will be resized to the size of the data passed
		to it. If it is false and the internal array is larger than the size passed in any
		extra elements will be left alone. If the internal array is smaller only enough data
		will be copied to fit so you may lose the end of the data passed in.
	 */
	void copyFrom (float* data, int size, bool resizeToFit = true)
	{
		if (resizeToFit)
			setSizeQuick (size);
		
		quickCopy (data, size);
	}

	/**	Applys the current buffer to a number of samples.
		This is done via a straight multiplication and will zero any out of range source samples.
	 */
	void applyBuffer (float* samples, int numSamples)
    {
        const int numToApply = jmin (bufferSize, numSamples);
     
        for (int i = 0; i < numToApply; i++)
            samples[i] *= buffer[i];
        
        if (bufferSize < numSamples)
            zeromem (samples + numToApply, (numSamples - numToApply) * sizeof (float));
    }
	
	/**	This performs a very quick copy of some data given to it.
		No resizing is done so if the size of the data passed is less than or equal to the size of the internal array
		it will be filled, otherwise elements will be left.
	 */
	void quickCopy (float* data, int size)
	{
		if (size > bufferSize)
			size = bufferSize;
		
		memcpy (buffer, data, size * sizeof (float));
	}
	
	/** Updates the buffer's listeners.
		Call this to explicitly tell any registerd listeners that the buffer has changed.
	 */
	void updateListeners()
    {
        listeners.call (&Listener::bufferChanged, this);
    }
	
	//==============================================================================
    /** Receives callbacks when a Buffer object changes.
		@see Buffer::addListener
	 */
    class  Listener
    {
    public:
        Listener()          {}
        virtual ~Listener() {}
		
        /** Called when a Buffer object is changed.
		 */
        virtual void bufferChanged (Buffer* buffer) = 0;
    };
	
    /** Adds a listener to receive callbacks when the buffer changes. */
    void addListener (Listener* const listener)         { listeners.add (listener); }
	
    /** Removes a listener that was previously added with addListener(). */
    void removeListener (Listener* const listener)      { listeners.remove (listener); }
	
private:
    //==============================================================================
	HeapBlock<float> buffer;
	int bufferSize;
	
    ListenerList<Listener> listeners;
	
	JUCE_LEAK_DETECTOR (Buffer)
};

#endif //__DROWAUDIO_BUFFER_H__