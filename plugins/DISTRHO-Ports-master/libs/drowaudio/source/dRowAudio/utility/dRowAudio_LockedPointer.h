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

#ifndef __DROWAUDIO_LOCKEDPOINTER_H__
#define __DROWAUDIO_LOCKEDPOINTER_H__

//==============================================================================
/**
    Wrapper for a pointer that automatically locks a provided lock whilst the LockedPointer
    stays in scope. This is similar using a narrowly scoped ScopedPointer.
 
    @code
        if ((LockedPointer<MyPointerType, SpinLock> pointer (getObject(), getLock()) != nullptr)
        {
            // object is now safely locked
            pointer->unthreadSafeMethodCall();
        }
 
        // as pointer goes out of scope the lock will be released
    @endcode
 */
template<typename Type, typename LockType>
class LockedPointer
{
public:
    //==============================================================================
    /** Creates a LockedPointer for a given pointer and corresponding lock.
        You can create one of these on the stack to makes sure the provided lock is 
        locked during all operations on the object provided.
     */
    LockedPointer (Type* pointer_, LockType& lock_)
        : pointer       (pointer_),
          lock          (lock_),
          scopedLock    (lock)
    {
        jassert (pointer != nullptr);
    }
    
    /** Destructor.
        Will safely unlock the lock passed in.
     */
    ~LockedPointer()
    {}
    
    /** Provides access to the objects methods. */
    Type* operator->()                      { return pointer; }
    
    /** Provides access to the objects methods. */
    Type const* operator->() const          { return pointer; }
    
    /** Returns the type of scoped lock to use for locking this pointer. */
    typedef typename LockType::ScopedLockType ScopedLockType;

private:
    //==============================================================================
    Type* pointer;
    LockType& lock;
    const ScopedLockType scopedLock;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LockedPointer);
};


#endif //__DROWAUDIO_LOCKEDPOINTER_H__
