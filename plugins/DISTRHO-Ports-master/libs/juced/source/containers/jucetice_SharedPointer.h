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

#ifndef __JUCETICE_SHAREDPOINTER_JUCEHEADER__
#define __JUCETICE_SHAREDPOINTER_JUCEHEADER__


//==============================================================================
/*
    Implementation of a shared pointer
*/

template<class PointerType,
         class TypeOfCriticalSectionToUse = DummyCriticalSection>
class SharedPointer
{
public:

    //==============================================================================
    SharedPointer()
        : data (0),
          count (0) 
    {
        // Increment the reference count
        count++;
    }

    SharedPointer (PointerType* value)
        : data (value),
          count (0)
    {
        // Increment the reference count
        count++;
    }

    SharedPointer (const SharedPointer<PointerType>& sp)
        : data (0),
          count (0)
    {
        if (this != &sp)
        {
            lock.enter();

            data = sp.data;
            count = sp.count;

            // Increment the reference count
            count++;

            lock.exit();
        }
    }

    ~SharedPointer()
    {
        lock.enter();

        if (data != 0)
        {
            if (--count == 0)
                deleteAndZero (data);
        }
        
        lock.exit();
    }

    PointerType& operator* ()
    {
        return *data;
    }

    PointerType* operator-> ()
    {
        return data;
    }

    PointerType* get ()
    {
        return data;
    }

    SharedPointer<PointerType>& operator = (const SharedPointer<PointerType>& sp)
    {
        if (this != &sp) // Avoid self assignment
        {
            lock.enter();

            // Decrement the old reference count
            // if reference become zero delete the old data
            if (--count == 0)
                delete data;

            // Copy the data and reference pointer
            // and increment the reference count
            data = sp.data;
            count = sp.count;

            count++;
            
            lock.exit();
        }

        return *this;
    }

    int used ()
    {
        return count;
    }

private:

    PointerType* data;
    TypeOfCriticalSectionToUse lock;
    mutable int count;
};

#endif
