/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2009 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2007 by Julian Storer.

 ------------------------------------------------------------------------------

 JUCE and JUCETICE can be redistributed and/or modified under the terms of
 the GNU General Public License, as published by the Free Software Foundation;
 either version 2 of the License, or (at your option) any later version.

 JUCE and JUCETICE are distributed in the hope that they will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with JUCE and JUCETICE; if not, visit www.gnu.org/licenses or write to
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA

 ==============================================================================
*/

#ifndef __JUCETICE_LOCKFREEFIFO_HEADER__
#define __JUCETICE_LOCKFREEFIFO_HEADER__

//==============================================================================
/**
 * Exception to catch in the NOT - REALTIME thread polling for get !
 */
class LockFreeFifoUnderrunException
{
public:
    LockFreeFifoUnderrunException ()
    {
    }
};

class LockFreeFifoOverrunException
{
public:
    LockFreeFifoOverrunException ()
    {
    }
};

 
//==============================================================================
/*
    Implementation of a circular buffer
 
    - <typename T> specifies the samples type (i.e. use <float> for VST)
    - The buffer is memory aligned for better performance
*/
template<class ElementType>
class LockFreeFifo
{
public:

    //==============================================================================
    /** Constructor */
    LockFreeFifo (int32 bufferSize_)
        : readIndex (0),
          writeIndex (0),
          bufferSize (bufferSize_),
          buffer (0)
    {
        buffer = new ElementType [bufferSize];
    }

    ~LockFreeFifo ()
    {
        delete[] buffer;
    }

    //==============================================================================
    /**
        Get a value from the FIFO
    */
    ElementType get ()
    {
        if (readIndex == writeIndex)
        {
            return (ElementType) 0;
        }

        ElementType data = buffer [readIndex];

        readIndex = (readIndex + 1) % bufferSize;

        return data;
    }

    //==============================================================================
    /**
        Feed in a new value to the FIFO
    */
    void put (ElementType data)
    {
        int32 newIndex = (writeIndex + 1) % bufferSize;

        if (newIndex == readIndex)
        {
            return;
        }

        buffer [writeIndex] = data;

        writeIndex = newIndex;
    }

    //==============================================================================
    /**
        Returns true if the fifo is empty
    */
    bool isEmpty () const 
    {
        return readIndex == writeIndex;   
    }

    /**
        Returns true if the fifo is full
    */
    bool isFull () const 
    {
        return ((writeIndex + 1) % bufferSize) == readIndex;   
    }

private:

    volatile int32 readIndex, writeIndex, bufferSize;
    ElementType* buffer;
};

#endif

