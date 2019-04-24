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
 
 This file is based around Niall Moody's OSC/UDP library, but was modified to
 be more independent and modularized, using the best practices from JUCE.
 
 ==============================================================================
 */

#ifndef __JUCETICE_OPENSOUNDBUNDLE_HEADER__
#define __JUCETICE_OPENSOUNDBUNDLE_HEADER__

#include "jucetice_OpenSoundBase.h"
#include "jucetice_OpenSoundTimeTag.h"


class OpenSoundMessage;

///    Class representing an OSC Bundle.
/*!
 Remember, bundles can be recursive, so a bundle may contain other bundles,
 as well as messages.
 */
class OpenSoundBundle : public OpenSoundBase
{
public:
    ///    Default constructor.
    /*!
	 Typically used when you're constructing a bundle to send out.
     */
    OpenSoundBundle();
    ///    Constructor used to create a bundle and it's contents from a block of char data.
    /*!
	 \param data A block of data which should contain an OSC bundle.
	 \param size The size of the data.
	 
	 You'd typically use this when you've received a block of data, and want
	 to extract all the bundle data from it.  To be safe, make sure you check
	 the data is actually a bundle with the isBundle() static method before
	 you call this.
     */
    OpenSoundBundle (char *data, const int size);
    ///    Destructor.
    /*!
	 All elements in the messageArray and bundleArray are deleted here.
     */
    ~OpenSoundBundle();
	
    ///    Returns the total size of the message.
    /*!
	 You must call this beforeyou call getData(), or the memory for
	 getData()'s return buffer won't be allocated.
     */
    int32 getSize();
    
    ///    Returns the bundle (including all it's elements) as a contiguous block of memory.
    char *getData();
	
    ///    Adds an element to the bundle's array of contents.
    /*!
	 \param entry Pointer to the element to add.
	 \param deleteWhenUsed If true, the bundle will delete the message when
	 it's finished with it, otherwise it's the caller's responsibility.
	 
	 The element may be either a Message or a Bundle.  The bundle (this
	 bundle) will call Message::isMessage() and Bundle::isBundle() on the
	 element's data to determine what kind of entry it is.  This is
	 necessary so we can easily manipulate the data with the various
	 getMessage() and getBundle() methods.
	 
	 Note that the order you add elements is not preserved for the final
	 bundle sent off via getData() - the final order will always be:
	 1st->last Messages, 1st->last Bundles.
     */
    void addElement (OpenSoundBase *entry, bool deleteWhenUsed = true);
    
    ///    Adds a Message to the bundle's array of contents.
    /*!
	 This is a lot faster than addElement(), since it doesn't check whether
	 the element's a Message or not.
     */
    void addMessage (OpenSoundMessage *message, bool deleteWhenUsed);
    
    ///    Clears all elements from the bundle (deletes them).
    void clearElements();
    
    ///    Sets the time tag for this bundle.
    void setTimeTag (const TimeTag& val);
    
    ///    Returns the time tag for this bundle.
    TimeTag getTimeTag() const { return timeTag; }
	
    ///    Returns the number of elements in this bundle.
    long getNumElements() const;
	
    ///    Returns the number of Messages in this bundle.
    long getNumMessages() const {return messageArray.size();};
	
    ///    Returns the number of Bundles in this bundle.
    long getNumBundles() const {return bundleArray.size();};
	
    ///    Returns the indexed Message.
    /*!
	 \return 0 if we're outside the array's bounds.
     */
    OpenSoundMessage *getMessage(const long index);
	
    ///    Returns the indexed Bundle.
    /*!
	 \return 0 if we're outside the array's bounds.
     */
    OpenSoundBundle *getBundle(const long index);
	
    ///    Method to check whether a block of data is an OSC Message.
    /*!
	 Basically just checks for the presence of the initial '#bundle' string
	 that all OSC bundles should have as their first bytes.
     */
    static bool isBundle(char *data, const int size);
	
private:
	
    TimeTag timeTag;
    std::vector<OpenSoundMessage *> messageArray;
    std::vector<bool> messageShouldBeDeleted;
    std::vector<OpenSoundBundle *> bundleArray;
    std::vector<bool> bundleShouldBeDeleted;
    int32 bufferSize;
    int32 outgoingSize;
    char *dataBuffer;
};


#endif

