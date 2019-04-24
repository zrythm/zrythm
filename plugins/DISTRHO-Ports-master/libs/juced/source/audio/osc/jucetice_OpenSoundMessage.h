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

#ifndef __JUCETICE_OPENSOUNDMESSAGE_HEADER__
#define __JUCETICE_OPENSOUNDMESSAGE_HEADER__

#include "jucetice_OpenSoundBase.h"

///    Class representing an OSC message.
class OpenSoundMessage : public OpenSoundBase
{
public:

    ///    Default constructor.
    /*!
        Typically used when you're constructing a Message to send out.
     */
    OpenSoundMessage();

    ///    Constructor to create a Message from a block of char data.
    /*!
        \param data The data to construct the Message from.
        \param size The size of the data.

        You'll typically use this to construct a Message from a block of data
        you've received.  To be safe, make sure you check the data is a Message
        with the isMessage() static method first.
     */
    OpenSoundMessage (char *data, const int size);

    ///    Destructor.
    ~OpenSoundMessage();

    ///    Returns the total size of the message.
    /*!
        You must call this before you call getData(), or the memory for
        getData()'s return buffer won't be allocated.
     */
    int32 getSize();
    
    ///    Returns the message as a contiguous block of memory.
    char *getData();

    ///    Clears the message data (erases the various vectors).
    void clearMessage();

    ///    Sets the message's destination address.
    void setAddress (const String& val);

    ///    Adds an Int32 value to the message.
    void addInt32 (const int32 val);

    ///    Adds an Float32 value to the message.
    void addFloat32 (const float val);

    ///    Adds an OscString value to the message.
    void addOscString (const String& val);

    ///    Returns the address of the Message.
    String getAddress() const { return address; }

    ///    Returns the type tag of the Message.
    String getTypeTag() const { return typeTag; }

    ///    Returns the number of floats in this Message.
    int getNumFloats() const { return floatArray.size(); }

    ///    Returns the indexed float.
    float getFloat(int32 index) const { return floatArray[index]; }

    ///    Returns the number of ints in this Message.
    int getNumInts() const { return intArray.size(); };

    ///    Returns the indexed int.
    int32 getInt(int32 index) const { return intArray[index]; }

    ///    Returns the number of strings in this Message.
    int getNumStrings() const { return stringArray.size(); }

    ///    Returns the indexed string.
    String getString (int32 index) const { return stringArray[index]; }

    ///    Method to check whether a block of data is an OSC Message.
    /*!
        This is a very simplistic check, and could be greatly improved.  For
        now, there's a very good chance it'll generate false positives if given
        packets which start with '/' and have a ',' somewhere in them.
     */
    static bool isMessage (char *data, const int size);

private:

    String address;
    String typeTag;
    Array<int32> intArray;
    Array<float> floatArray;
    StringArray stringArray;
    int32 bufferSize;
    int32 outgoingSize;
    char *buffer;
};

#endif
