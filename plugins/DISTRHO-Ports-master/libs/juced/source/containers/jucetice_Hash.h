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

#ifndef __JUCETICE_HASH_JUCEHEADER__
#define __JUCETICE_HASH_JUCEHEADER__

//==============================================================================
/** The default max load (percentage) of the total size.

    When we reach numElements > (maxsize * juceDefaultHashMaxLoad) then we
    will grow the array a bit more.
*/
const float juceDefaultHashMaxLoad = 0.66f;

//==============================================================================
/** The default initial size of the owned hash internal table

    Could be a prime number, better if is a prime (but not mandatory)
*/
const int juceDefaultHashDefaultSize = 101;


//==============================================================================
/**
    Implements a hash typically useful with default types

    It is faster when accessing elements with bigger tables than doing a linear
    search of an unordered table.

    @code
        class StringHashFunction {
        public:
            static int generateHash (const String& key, const int size) {
                return key.hashCode() % size;
            }
        };
        typedef Hash<String, int, StringHashFunction> StringHash;

        // usage example
        StringHash hash;
        hash.add ("test1", 1);
        hash.add ("test2", 2);

        if (hash.contains ("test1"))
            std::cout << hash ["test1"] << std::endl;

        hash.remove ("test2");
        if (hash.contains ("test2"))
            std::cout << hash ["test2"] << std::endl;

    @endcode

    @see OwnedHash
*/
template <class KeyType,
          class ObjectType,
          class HashFunctionToUse,
          class TypeOfCriticalSectionToUse = DummyCriticalSection>
class Hash
{
private:

    //==============================================================================
    /** Internal linked list structure holding key and value */
    struct HashEntry
    {
        HashEntry (const KeyType key_, ObjectType element_)
        {
            key = key_;
            element = element_;
            nextEntry = 0;
        }

        KeyType key;
        ObjectType element;
        HashEntry* nextEntry;
    };

public:

    //==============================================================================
    /** Iterates the owned hash regardless of the order

        @see OwnedHash
    */
    class Iterator
    {
    public:
        //==============================================================================
        Iterator (const Hash& set_)
            : currentEntry (0),
              set (set_),
              currentRowEntry (0),
              currentElement (0)
        {
        }

        //==============================================================================
        /** Moves onto the next element in the hash

            If this returns false, there are no more elements. If it returns true,
            the currentElement variable will be set to the hold pointer of the current
            element, while currentKey will hold it's key
        */
        bool next()
        {
            // advance to the next entry
            if (currentEntry)
                currentEntry = currentEntry->pNext;

            // if the entry does not exist, scan for a non empy row
            while (! currentEntry && currentRowEntry < set.size() - 1)
                currentEntry = set.entries [++currentRowEntry];

            if (currentEntry)
            {
                currentKey = currentEntry->key;
                currentElement = currentEntry->element;
                return true;
            }
            else
            {
                currentKey = KeyType();
                currentElement = 0;
                return false;
            }
        }

        //==============================================================================
        const KeyType& currentKey;
        ObjectType currentElement;

    private:
        const Hash& set;
        int currentRowEntry;
        HashEntry* currentEntry;

        Iterator (const Iterator&);
        const Iterator& operator= (const Iterator&);
    };


    //==============================================================================
    /** Creates an empty owned hash with intial size of juceDefaultOwnedHashDefaultSize. */
    Hash ()
        : maxSize (0),
          numEntries (0),
          entries (0)
    {
        setAllocatedSize (juceDefaultHashDefaultSize);
    }

    /** Creates an owned hash with the initial size of the specified value.

        @param initialSize  this is the size of increment by which the internal storage
        will be increased.
    */
    Hash (const int initialSize)
        : maxSize (0),
          numEntries (0),
          entries (0)
    {
        setAllocatedSize (initialSize);
    }

    /** Destructor. */
    ~Hash()
    {
        clear ();
        setAllocatedSize (0);
    }

    //==============================================================================
    /** Removes all elements from the owned hash.

        This will remove all the elements, and free any storage that the hash is
        using. You can eventually block deleting owned object by passing false as
        first argument (it works like clear of the OwnedArray).
    */
    void clear ()
    {
        const ScopedLock sl (lock);

        for (int i = 0; i < maxSize; i++)
        {
            HashEntry* entry = entries[i];

            while (entry)
            {
                HashEntry* oldEntry = entry;
                entry = entry->nextEntry;

                delete oldEntry;
            }

            entries[i] = 0;
        }
        
        numEntries = 0;
    }

    //==============================================================================
    /** Returns the current number of elements in the owned hash. */
    inline int size() const throw()
    {
        return numEntries;
    }

    /** If the key passed in is not present  elements, this will return zero.

        If you're certain that the key will always be a valid element, you
        can call getUnchecked() instead, which is faster.

        @param keyToLookFor    the key of the element being requested
        @see getUnchecked
    */
    inline ObjectType operator[] (const KeyType keyToLookFor) const throw()
    {
        const ScopedLock sl (lock);

        const int searchIndex = HashFunctionToUse::generateHash (keyToLookFor, maxSize);
        HashEntry* entry = entries [searchIndex];

        while (entry && entry->key != keyToLookFor)
            entry = entry->nextEntry;

        ObjectType result = (entry != 0) ? entry->element : 0;

        return result;
    }

    /** Returns a pointer to the object with this key in the map, without checking if
        it exists or not.

        This is a faster and less safe version of operator[] which doesn't check the
        resulting entry if it exists or not, so it can be used when you're sure the
        key will always represent a object in the map.
    */
    inline ObjectType getUnchecked (const KeyType keyToLookFor) const throw()
    {
        const ScopedLock sl (lock);

        const int searchIndex = HashFunctionToUse::generateHash (keyToLookFor, maxSize);
        HashEntry* entry = entries [searchIndex];

        while (entry && entry->key != keyToLookFor)
            entry = entry->nextEntry;

        ObjectType result = entry->element;

        return result;
    }

    //==============================================================================
    /** Returns the key of the given object.

        @param elementToLookFor     the value or object to look for
        @param resultKey            the key for the found object (unset otherwise)
        @returns                    true if we found the key for the object
    */
    bool keyOf (const ObjectType& elementToLookFor, KeyType& resultKey) const throw()
    {
        const ScopedLock sl (lock);

        for (int i = maxSize; --i >= 0;)
        {
            HashEntry* entry = entries[i];
            while (entry)
            {
                if (entry->element == elementToLookFor)
                {
                    resultKey = entry->key;
                    return true;
                }

                entry = entry->nextEntry;
            }
        }
        
        return false;
    }

    /** Returns true if the owned hash contains one occurrence of the specied key.

        @param keyToLookFor         the key to look for
        @returns                    true if the item is found
    */
    bool contains (const KeyType keyToLookFor) const throw()
    {
        const ScopedLock sl (lock);

        const int searchIndex = HashFunctionToUse::generateHash (keyToLookFor, maxSize);
        HashEntry* entry = entries [searchIndex];

        while (entry && entry->key != keyToLookFor)
            entry = entry->nextEntry;

        return entry != 0;
    }

    /** Returns true if the owned hash contains at least one occurrence of an object.

        @param elementToLookFor     the value or object to look for
        @returns                    true if the item is found
    */
    bool containsValue (const ObjectType& elementToLookFor) const throw()
    {
        const ScopedLock sl (lock);

        for (int i = maxSize; --i >= 0;)
        {
            HashEntry* entry = entries[i];
            while (entry)
            {
                if (entry->element == elementToLookFor)
                    return true;

                entry = entry->nextEntry;
            }
        }

        return false;
    }

    //==============================================================================
    /** Appends a new element to the owned hash.

        @param newElement       the new object to add to the owned hash
        @see remove
    */
    void add (const KeyType newKey, const ObjectType& newElement) throw()
    {
        const ScopedLock sl (lock);

        const int writeIndex = HashFunctionToUse::generateHash (newKey, maxSize);
        HashEntry* entry = entries [writeIndex];

        if (! entry)
        {
            insertAt (newKey, newElement, writeIndex);
        }
        else
        {
            while (entry->nextEntry && entry->nextEntry->key != newKey)
                entry = entry->nextEntry;

            if (! entry->nextEntry)
                insertAt (newKey, newElement, writeIndex);
        }
    }

    /** Removes an element from the owned hash.

        This will remove the element, and eventually free the holded object.
        If the key passed is not currently in the owned hash, nothing will happen.

        @param keyToRemove      the key of the element to remove
        @param deleteObject     true if ou want to free the holded object memory
    */
    void remove (const KeyType keyToRemove) throw ()
    {
        const ScopedLock sl (lock);

        const int removeIndexPos = HashFunctionToUse::generateHash (keyToRemove, maxSize);
        HashEntry* entry = entries [removeIndexPos];

        if (entry)
        {
            // it's the first one
            if (entry->key == keyToRemove)
            {
                entries [removeIndexPos] = entry->nextEntry;

                delete entry;
                --numEntries;
            }
            else
            {
                while (entry->nextEntry && entry->nextEntry->key != keyToRemove)
                    entry = entry->nextEntry;

                // we found it
                if (entry->nextEntry)
                {
                    HashEntry* tempEntry = entry->nextEntry;
                    entry->nextEntry = entry->nextEntry->nextEntry;

                    delete tempEntry;
                    --numEntries;
                }
            }
        }
    }

    /** Removes a specified object from the owned hash.

        If the item isn't found, no action is taken.

        @param objectToRemove   the object to try to remove
        @param deleteObject     whether to delete the object (if it's found)
        @see remove, removeRange
    */
    void removeValue (const ObjectType& objectToRemove)
    {
        const ScopedLock sl (lock);

        bool continueSearch = true;

        for (int i = maxSize; --i >= 0 && continueSearch;)
        {
            HashEntry* entry = entries[i];
            HashEntry* previousEntry = 0;
            while (entry)
            {
                if (entry->element == objectToRemove)
                {
                    if (previousEntry == 0)
                        entries [i] = entry->nextEntry;
                    else
                        previousEntry->nextEntry = entry->nextEntry;

                    delete entry;
                    --numEntries;

                    continueSearch = false;
                    break;
                }

                previousEntry = entry;
                entry = entry->nextEntry;
            }
        }
    }

    //==============================================================================
    /** Swaps a pair of objects in the owned hash.

        If either of the keys passed in are not found, nothing will happen,
        otherwise the two objects with these keys will be exchanged.
    */
    void swap (const KeyType keyIndex1,
               const KeyType keyIndex2) throw()
    {
        const ScopedLock sl (lock);

        const int indexSearch1 = HashFunctionToUse::generateHash (keyIndex1, maxSize);
        const int indexSearch2 = HashFunctionToUse::generateHash (keyIndex2, maxSize);
        HashEntry* entry1 = entries [indexSearch1];
        HashEntry* entry2 = entries [indexSearch2];

        while (entry1)
        {
            if (entry1->key == keyIndex1)
                break;
            entry1 = entry1->nextEntry;
        }

        while (entry2)
        {
            if (entry2->key == keyIndex2)
                break;
            entry2 = entry2->nextEntry;
        }

        if (entry1 && entry2)
            swapVariables (entry1->element, entry2->element);
    }

    //==============================================================================
    /** Locks the owned hash's CriticalSection.

        Of course if the type of section used is a DummyCriticalSection, this won't
        have any effect.

        @see unlockHash
    */
    void lockHash() const throw()
    {
        lock.enter();
    }

    /** Unlocks the owned hash's CriticalSection.

        Of course if the type of section used is a DummyCriticalSection, this won't
        have any effect.

        @see lockHash
    */
    void unlockHash() const throw()
    {
        lock.exit();
    }

    //==============================================================================
    juce_UseDebuggingNewOperator

private:

    //==============================================================================
    /** Changes the amount of storage allocated.

        This will retain any data currently held in the owned hash

        @param newSize    the size of the table to create
    */
    void setAllocatedSize (const int newSize) throw()
    {
        if (maxSize != newSize)
        {
            maxSize = newSize;

            if (newSize > 0)
            {
                entries = (HashEntry**) ::malloc (sizeof (HashEntry*) * newSize);
                zeromem (entries, sizeof (HashEntry*) * newSize);
            }
            else
            {
                if (entries != 0)
                {
                    juce_free (entries);
                    entries = 0;
                }
            }
        }
    }

    //==============================================================================
    void insertAt (const KeyType newKey, const ObjectType& newElement, const int pushPosition)
    {
        HashEntry* newEntry = new HashEntry (newKey, newElement);
        HashEntry* oldEntry = entries [pushPosition];
        entries [pushPosition] = newEntry;
        newEntry->nextEntry = oldEntry;
        numEntries++;

        if ((numEntries / (float) maxSize) > juceDefaultHashMaxLoad)
            growInternalArray();
    }

    //==============================================================================
    void growInternalArray()
    {
#ifdef JUCE_DEBUG
        const int oldNumEntries = numEntries;
#endif
        const int oldSize = maxSize;
        HashEntry** oldEntries = entries;

        // create new storage twice as big as the old
        setAllocatedSize (2 * maxSize);

        // rehash all of the entries
        // this can be more efficient-- right now it recreates all of the hash_entries.
        for (int i = 0; i < oldSize; i++)
        {
            HashEntry* entry = oldEntries[i];
            while (entry)
            {
                add (entry->key, entry->element);
                HashEntry* temp = entry;
                entry = entry->nextEntry;
                delete temp;
            }
        }

#ifdef JUCE_DEBUG
        jassert (oldNumEntries == numEntries);
#endif

        juce_free (oldEntries);
    }

    //==============================================================================
    friend class Iterator;

    int maxSize, numEntries;
    HashEntry** entries;
    TypeOfCriticalSectionToUse lock;

    Hash (const Hash&);
    const Hash& operator= (const Hash&);
};


/*

//==============================================================================
class StringHashFunction
{
public:

    static int generateHash (const String& key, const int size)
    {
        return key.hashCode() % size;
    }
};

typedef OwnedHash<String, String, StringHashFunction> StringOwnedHash;


//==============================================================================
class IntHashFunction
{
public:

    static int generateHash (int e, int size)
    {
        if (e < 0)
            e *= -1;
        return e % size;
    }
};

typedef OwnedHash<int, String, IntHashFunction> IntOwnedHash;

*/

#endif

