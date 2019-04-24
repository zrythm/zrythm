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

#ifdef WIN32
  #include <winsock.h>
#else
  #include <netinet/in.h>
#endif

BEGIN_JUCE_NAMESPACE

//------------------------------------------------------------------------------
OpenSoundMessage::OpenSoundMessage()
  : OpenSoundBase(),
    bufferSize(0),
    outgoingSize(0),
    buffer(0)
{
    typeTag = ",";
}

//------------------------------------------------------------------------------
OpenSoundMessage::OpenSoundMessage(char *data, const int size)
  : OpenSoundBase(),
    bufferSize(0),
    outgoingSize(0),
    buffer(0)
{
    int i;
    GetTheBytes tempBytes;
    int32 tempint;
    float *tempf;
    String tempstr;
    //This represents which part of the Message we're currently reading from.
    int currentSection = 0;
    int currentTag = 1; //Because Type Tags start with ','.

    for(i=0;i<size;++i)
    {
        //Address.
        if(currentSection == 0)
        {
            if((data[i] != 0) && (data[i] != ','))
                address += data[i];
            else if(data[i] == ',')
            {
                typeTag = ",";
                ++currentSection;
            }
        }
        //Type Tag.
        else if(currentSection == 1)
        {
            if(data[i] != 0)
                typeTag += data[i];
            else if((i%4) == 3) //i.e. we're on the last byte of a 4 byte boundary.
                ++currentSection;
        }
        //The actual data.
        else
        {
            if(currentTag >= static_cast<int>(typeTag.length()))
                break;
            else if(typeTag[currentTag] == 'f')
            {
                tempBytes.a = data[i];
                ++i;
                tempBytes.b = data[i];
                ++i;
                tempBytes.c = data[i];
                ++i;
                tempBytes.d = data[i];

                tempint = ntohl(*(reinterpret_cast<int32 *>(&tempBytes)));
                tempf = reinterpret_cast<float *>(&tempint);
                floatArray.add (*tempf);

                ++currentTag;
            }
            else if(typeTag[currentTag] == 'i')
            {
                tempBytes.a = data[i];
                ++i;
                tempBytes.b = data[i];
                ++i;
                tempBytes.c = data[i];
                ++i;
                tempBytes.d = data[i];

                tempint = ntohl(*(reinterpret_cast<int32 *>(&tempBytes)));
                intArray.add (tempint);

                ++currentTag;
            }
            else if(typeTag[currentTag] == 's')
            {
                tempstr = "";
                while(data[i] != 0)
                {
                    tempstr += data[i];

                    ++i;
                }

				//@TODO
				//Not sure with this, it was
				//tempstr[tempstr.length()] = static_cast<char>(NULL); //Terminator.

				tempstr = tempstr.replaceCharacter (tempstr.getLastCharacter(),
													static_cast<char>(NULL)); //Terminator.

                stringArray.add (tempstr);

                //Handle any padding bytes.
                while((i%4) != 0)
                    ++i;

                ++currentTag;
            }
        }
    }
}

//------------------------------------------------------------------------------
OpenSoundMessage::~OpenSoundMessage()
{
    if(buffer)
    {
        delete [] buffer;
        buffer = 0;
    }
}

//------------------------------------------------------------------------------
int32 OpenSoundMessage::getSize()
{
    int32 i;
    int32 tempint;

    outgoingSize = 0;
    if((address.length()+1)%4)
    {
        //Because we need space for the padding bytes.
        tempint = (4 - ((address.length()+1)%4));
        outgoingSize += address.length() + 1;
        outgoingSize += tempint;
    }
    else
        outgoingSize += address.length() + 1; //+1 = Null terminator.
    if((typeTag.length()+1)%4)
    {
        //Because we need space for the padding bytes.
        tempint = (4 - ((typeTag.length()+1)%4));
        outgoingSize += typeTag.length() + 1;
        outgoingSize += tempint;
    }
    else
        outgoingSize += typeTag.length() + 1; //+1 = Null terminator.

    outgoingSize += intArray.size() * sizeof(int32);
    outgoingSize += floatArray.size() * sizeof(float);

    for(i=0;i<static_cast<int32>(stringArray.size());++i)
    {
        if((stringArray[i].length()+1)%4)
        {
            //Because we need space for the padding bytes.
            tempint = (4 - ((stringArray[i].length()+1)%4));
            outgoingSize += stringArray[i].length() + 1;
            outgoingSize += tempint;
        }
        else
            outgoingSize += stringArray[i].length() + 1; //+1 = Null terminator.
    }

    if(outgoingSize > bufferSize)
    {
        bufferSize = outgoingSize;
        if(buffer)
        {
            delete [] buffer;
            buffer = 0;
        }

        buffer = new char[bufferSize];
    }

    return outgoingSize;
}

//------------------------------------------------------------------------------
char *OpenSoundMessage::getData()
{
    int32 addressCount = 0;
    int32 typeTagCount = 0;
    int32 intCount = 0;
    int32 floatCount = 0;
    int32 stringCount = 0;
    int32 currentTypeTag = 1;
    int32 i, j;
    GetTheBytes *tempBytes;
    int32 tempInt;
    float tempFloat;
    int32 addressLength = static_cast<int32>(address.length());
    int32 typeTagLength = static_cast<int32>(typeTag.length());
    int32 stringLength;

    if(!buffer)
        return 0;

    for(i=0;i<outgoingSize;++i)
    {
        //If we're currently writing the address to the buffer.
        if(addressCount < (addressLength + 1))
        {
            buffer[i] = address[addressCount];
            ++addressCount;

            if(addressCount == addressLength)
            {
                ++i;
                buffer[i] = static_cast<char>(NULL);
                ++addressCount;
                //Add any packing bytes.
                while(addressCount%4)
                {
                    ++i;
                    buffer[i] = static_cast<char>(NULL);
                    ++addressCount;
                }
            }
        }
        //If we're currently writing the type tag to the buffer.
        else if(typeTagCount < (typeTagLength + 1))
        {
            buffer[i] = typeTag[typeTagCount];
            ++typeTagCount;

            if(typeTagCount == typeTagLength)
            {
                ++i;
                buffer[i] = static_cast<char>(NULL);
                ++typeTagCount;
                //Add any packing bytes.
                while(typeTagCount%4)
                {
                    ++i;
                    buffer[i] = static_cast<char>(NULL);
                    ++typeTagCount;
                }
            }
        }
        //If we're currently writing part of the data to the buffer.
        else if(currentTypeTag < typeTagLength)
        {
            //Determine what kind of data we should be writing to the buffer.
            switch(typeTag[currentTypeTag])
            {
                //If we're writing an integer to the buffer.
                case 'i':
                    tempInt = htonl(intArray[intCount]);
                    tempBytes = reinterpret_cast<GetTheBytes *>(&tempInt);
                    buffer[i] = tempBytes->a;
                    ++i;
                    buffer[i] = tempBytes->b;
                    ++i;
                    buffer[i] = tempBytes->c;
                    ++i;
                    buffer[i] = tempBytes->d;

                    ++intCount;
                    break;
                //If we're writing a float to the buffer.
                case 'f': {
                    tempFloat = floatArray[floatCount];
                    int32* const _i((int32*)&tempFloat);
                    tempInt = htonl(*_i);
                    tempBytes = reinterpret_cast<GetTheBytes *>(&tempInt);
                    buffer[i] = tempBytes->a;
                    ++i;
                    buffer[i] = tempBytes->b;
                    ++i;
                    buffer[i] = tempBytes->c;
                    ++i;
                    buffer[i] = tempBytes->d;

                    ++floatCount;
                    break;
                }
                //If we're writing an OSC String to the buffer.
                case 's':
                    stringLength = static_cast<int32>(stringArray[stringCount].length());
                    for(j=1;j<stringLength;++j)
                    {
                        buffer[i] = stringArray[stringCount][j-1];
                        ++i;
                    }
                    buffer[i] = static_cast<char>(NULL);

                    //Add any packing bytes.
                    while(j%4)
                    {
                        ++i;
                        buffer[i] = static_cast<char>(NULL);
                        ++j;
                    }

                    ++stringCount;
                    break;
            }
            //Move on to the next tag in the message's type tag list.
            ++currentTypeTag;
        }
    }

    i = i;

    return buffer;
}

//------------------------------------------------------------------------------
void OpenSoundMessage::clearMessage()
{
    outgoingSize = 0;

    typeTag = ",";
    intArray.clear();
    floatArray.clear();
    stringArray.clear();
}

//------------------------------------------------------------------------------
void OpenSoundMessage::setAddress(const String& val)
{
    address = val;
}

//------------------------------------------------------------------------------
void OpenSoundMessage::addInt32(const int32 val)
{
    typeTag += 'i';
    intArray.add (val);
}

//------------------------------------------------------------------------------
void OpenSoundMessage::addFloat32(const float val)
{
    typeTag += 'f';
    floatArray.add (val);
}

//------------------------------------------------------------------------------
void OpenSoundMessage::addOscString(const String& val)
{
    typeTag += 's';
    stringArray.add (val);
}

//------------------------------------------------------------------------------
bool OpenSoundMessage::isMessage(char *data, const int size)
{
    int i;
    bool retval = false;

    //All OSC Messages start with the OSC address, which always starts with '/'.
    if(data[0] == '/')
    {
        for(i=1;i<size;++i)
        {
            //The comma marks the start of the Type Tag section of the message.
            //It'll always come at the start of a 4-byte boundary.
            if((data[i] == ',')&&((i%4) == 0))
            {
                retval = true;
                break;
            }
        }
    }

    return retval;
}

END_JUCE_NAMESPACE
