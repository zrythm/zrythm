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

#ifndef __DROWAUDIO_FIFOBUFFER_H__
#define __DROWAUDIO_FIFOBUFFER_H__

//==============================================================================
/**
    This is a simple implementation of a lock free Fifo buffer that uses
    a template parameter for the sample type. This should be a primitive type
    that is capable of being copied using only memcpy.
 
    Whilst read and write operations are atomic and son't strictly need locking
    there may be occasions where you are changing the size on another thread to
    read/write operation. To make the class completely thread safe you can
    optionally pass in a lock type. Of course if you are taking care of your own
    thread safety a DummyCriticalSection will be used which should be optimised
    out by the compiler.
 */
template <typename ElementType,
          typename TypeOfCriticalSectionToUse = DummyCriticalSection>
class FifoBuffer
{
public:
    //==============================================================================
    /** Creates a FifoBuffer with a given initial size. */
	FifoBuffer (int initialSize)
        : abstractFifo (initialSize)
	{
		buffer.malloc (abstractFifo.getTotalSize());
	}
	
    /** Returns the number of samples in the buffer. */
	inline int getNumAvailable()
	{
        const ScopedLockType sl (lock);
		return abstractFifo.getNumReady();
	}
	
    /** Returns the number of items free in the buffer. */
    inline int getNumFree()
    {
        const ScopedLockType sl (lock);
        return abstractFifo.getFreeSpace();
    }
    
    /** Sets the size of the buffer.
        This does not keep any of the old data and will reset the buffer
        making the number available 0. 
     */
	inline void setSize (int newSize)
	{
        const ScopedLockType sl (lock);
		abstractFifo.setTotalSize (newSize);
		buffer.malloc (abstractFifo.getTotalSize());
	}

    /** Sets the size of the buffer keeping as much of the
        existing data as possible.
     
        This is a potentially time consuming operation and is only thread
        safe if a valid lock is used as the second template parameter.
     */
	inline void setSizeKeepingExisting (int newSize)
	{
        const ScopedLockType sl (lock);

        const int numUsed = abstractFifo.getNumReady();
        
		abstractFifo.setTotalSize (newSize);
		buffer.realloc (newSize);
        abstractFifo.finishedWrite (numUsed);
	}

    /** Returns the size of the buffer. */
	inline int getSize()
	{
        const ScopedLockType sl (lock);
		return abstractFifo.getTotalSize();
	}
	
    inline void reset()
    {
        const ScopedLockType sl (lock);
        abstractFifo.reset();
    }
    
    /** Writes a number of samples into the buffer. */
	void writeSamples (const ElementType* samples, int numSamples)
	{
        const ScopedLockType sl (lock);

		int start1, size1, start2, size2;
		abstractFifo.prepareToWrite (numSamples, start1, size1, start2, size2);

		if (size1 > 0)
			memcpy (buffer.getData()+start1, samples, size1 * sizeof (ElementType));
		
		if (size2 > 0)
			memcpy (buffer.getData()+start2, samples+size1, size2 * sizeof (ElementType));

		abstractFifo.finishedWrite (size1 + size2);
	}
	
    /** Reads a number of samples from the buffer into the array provided. */
	void readSamples (ElementType* bufferToFill, int numSamples)
	{
        const ScopedLockType sl (lock);

		int start1, size1, start2, size2;
		abstractFifo.prepareToRead (numSamples, start1, size1, start2, size2);
		
		if (size1 > 0)
			memcpy (bufferToFill, buffer.getData() + start1, size1 * sizeof (ElementType));

		if (size2 > 0)
			memcpy (bufferToFill + size1, buffer.getData() + start2, size2 * sizeof (ElementType));
		
		abstractFifo.finishedRead (size1 + size2);
	}
	
    
    /** Removes a number of samples from the buffer. */
    void removeSamples (int numSamples)
    {
        const ScopedLockType sl (lock);
        abstractFifo.finishedRead (numSamples);
    }

    /** Returns the raw block of data.
        You shouldn't need to mess with this usually but could come in handy if you want
        to use the Fifo as a buffer without clearing it regularly.
     */
    ElementType* getData()                                  { return buffer.getData(); }
    
    //==============================================================================
    /** Returns the CriticalSection that locks this fifo.
        To lock, you can call getLock().enter() and getLock().exit(), or preferably use
        an object of ScopedLockType as an RAII lock for it.
     */
    inline const TypeOfCriticalSectionToUse& getLock() const noexcept      { return lock; }
    
    /** Returns the type of scoped lock to use for locking this fifo */
    typedef typename TypeOfCriticalSectionToUse::ScopedLockType ScopedLockType;

private:
    //==============================================================================
	AbstractFifo abstractFifo;
	HeapBlock<ElementType> buffer;
    TypeOfCriticalSectionToUse lock;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FifoBuffer);
};

#endif  // __DROWAUDIO_FIFOBUFFER_H__
