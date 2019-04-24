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

#ifndef __JUCETICE_CIRCULARBUFFER_JUCEHEADER__
#define __JUCETICE_CIRCULARBUFFER_JUCEHEADER__


//==============================================================================
/*
    Implementation of a circular buffer

    - <typename ElementType> specifies the samples type (i.e. use <float> for VST)
    - The buffer is memory aligned for better performance
*/
template<typename ElementType>
class CircularBuffer
{
public:

    //==============================================================================
    /** Constructor */
    CircularBuffer (uint32 length)
        : _length (length)
    {
        // allocate buffer (it will be at least int-aligned)
        _buffer = (ElementType*)_aligned_malloc(_length * sizeof(ElementType), (sizeof(int) > sizeof(ElementType)) ? sizeof(int) : sizeof(ElementType));
        // fills it w/zeros and init vars
        memset(_buffer, 0, _length * sizeof(ElementType));
        _firstFree = 0;
    }

    /** Destructor */
    ~CircularBuffer()
    {
        _aligned_free(_buffer);
    }

    //==============================================================================
    /** Fills the buffer with silence */
    void reset()
    {
        memset(_buffer, 0, _length * sizeof(ElementType));
        _firstFree = 0;
    }

    //==============================================================================
    /** Gets the length of the buffer passed to the constructor */
    int getSize() const
    {
        return _length;
    }

    //==============================================================================
    /** Feed in a new sample */
    void put (ElementType sample)
    {
        // Copy the sample to the first free location
        _buffer[_firstFree] = sample;
        // update feed position
        _firstFree = (_firstFree + 1) % _length;
    }

    /** Feed in a new <block> of samples of length <length> */
    void put (ElementType* block, uint32 length)
    {
        // if it doesn't fit, take last _length samples
        if(length > _length) {
            length = _length;                    // limit the length to the buffer's length
            block = &block[length - _length];    // start later, in order to get last <_length> samples
        }

        // if it has to be divided at the physical end of the buffer
        if(_firstFree + length > _length)
        {
            // fill the end of the buffer
            memcpy((_buffer + _firstFree), block, (_length - _firstFree) * sizeof(ElementType));
            // restart filling from the beginning of the buffer
            memcpy(_buffer, (block + _length - _firstFree), (length - _length + _firstFree) * sizeof(ElementType));
            // update feed position
            _firstFree = length - _length + _firstFree;
        }
        else
        {
            // Copy the entire block to the first free location
            memcpy((_buffer + _firstFree), block, length * sizeof(ElementType));
            // update feed position
            _firstFree = _firstFree + length;
        }
    }

    //==============================================================================
    /** Returns a sample feeded <samplesBackward> samples ago

        i.e. get(0) returns the last feeded sample
        get(getLength()-1) returns the oldest sample present in the buffer
    */
    ElementType get (uint32 samplesBackward) const
    {
        return _buffer[(_firstFree - samplesBackward - 1 + _length) % _length];
    }

    /** Fills <block> with the last <n> samples feeded into the buffer
        [out] <block> must have room for at least <n> samples
    */
    void getLast (ElementType * block, uint32 n) const
    {
        // cannot give away more than we have!
        n = (n < _length) ? n : _length;
        int start = (_firstFree + (_length - n)) % _length;
        int size = ((_length - start) < n) ? (_length - start) : n;
        memcpy(block, &_buffer[start], size * sizeof(ElementType));
        if(n - size)
            memcpy(&block[size], &_buffer[(start + size) % _length], (n - size) * sizeof(ElementType));
    }

private:

#ifndef _aligned_malloc
    static void* _aligned_malloc (size_t bytes, size_t alignment)
    {
        void *p1 ,*p2; // basic pointer needed for computation.

        if((p1 =(void *) malloc(bytes + alignment + sizeof(size_t)))==NULL)
            return NULL;

        size_t addr=(size_t)p1+alignment+sizeof(size_t);

        p2=(void *)(addr - (addr%alignment));

        *((size_t *)p2-1)=(size_t)p1;

        return p2;
    }
#endif

#ifndef _aligned_free
    static void _aligned_free (void *p)
    {
        /* Find the address stored by aligned_malloc ,"size_t" bytes above the
           current pointer then free it using normal free routine provided by C. */
        free((void *)(*((size_t *) p-1)));
    }
#endif

    ElementType* _buffer;         // the buffer
    uint32 _length;     // size of the buffer in samples
    uint32 _firstFree;  // next feed location index
};

#endif
