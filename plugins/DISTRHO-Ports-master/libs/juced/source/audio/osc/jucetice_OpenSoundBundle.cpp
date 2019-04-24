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

#include "jucetice_OpenSoundMessage.h"



//------------------------------------------------------------------------------
OpenSoundBundle::OpenSoundBundle()
: OpenSoundBase(),
bufferSize(0),
outgoingSize(0),
dataBuffer(0)
{
	
}

//------------------------------------------------------------------------------
OpenSoundBundle::OpenSoundBundle (char *data, const int size)
: OpenSoundBase(),
bufferSize(0),
outgoingSize(0),
dataBuffer(0)
{
    int i;
    int32 tempSize;
    TimeTag *tempTag;
    OpenSoundMessage *newMessage;
    OpenSoundBundle *newBundle;
	
    //Start at 8, because we don't care about the first '#bundle' string.
    for (i = 8; i < size; ++i)
    {
        if (i < 16)
        {
            //Get timeTag.
            tempTag = new TimeTag(data+i);
            timeTag = *tempTag;
            delete tempTag;
            i += 7;
        }
        else
        {
            tempSize = ntohl(*(reinterpret_cast<int32 *>(data+i)));
            i += 4;
			
            if(isBundle((data+i), tempSize))
            {
                newBundle = new OpenSoundBundle((data+i), tempSize);
                addElement(newBundle);
                i += (tempSize-1); //-1 because it gets incremented at the end of the loop.
            }
            else if(OpenSoundMessage::isMessage((data+i), tempSize))
            {
                newMessage = new OpenSoundMessage((data+i), tempSize);
                addElement(newMessage);
                i += (tempSize-1); //-1 because it gets incremented at the end of the loop.
            }
        }
    }
}

//------------------------------------------------------------------------------
OpenSoundBundle::~OpenSoundBundle()
{
    clearElements();
	
    if(dataBuffer)
    {
        delete [] dataBuffer;
        dataBuffer = 0;
    }
}

//------------------------------------------------------------------------------
int32 OpenSoundBundle::getSize()
{
    std::vector<OpenSoundMessage *>::iterator itM;
    std::vector<OpenSoundBundle *>::iterator itB;
    int32 retval = 0;
	
    retval += 8; //'#bundle'
    retval += 8; //timeTag
	
    for(itM=messageArray.begin();itM!=messageArray.end();++itM)
    {
        retval += 4; //For the size of the message.
        retval += (*itM)->getSize();
    }
	
    for(itB=bundleArray.begin();itB!=bundleArray.end();++itB)
    {
        retval += 4; //For the size of the bundle.
        retval += (*itB)->getSize();
    }
	
    outgoingSize = retval;
    if(outgoingSize > bufferSize)
    {
        //This shouldn't ever happen, but just in case...
        if(outgoingSize < 16)
            outgoingSize = 16;
		
        bufferSize = outgoingSize;
        if(dataBuffer)
        {
            delete [] dataBuffer;
            dataBuffer = 0;
        }
		
        dataBuffer = new char[bufferSize];
    }
	
    return retval;
}

//------------------------------------------------------------------------------
char *OpenSoundBundle::getData()
{
    int32 i, j;
    int32 tempInt;
    unsigned long tempL;
    GetTheBytes *tempBytes;
    char *tempBuffer;
    int32 messageCount = 0;
    int32 bundleCount = 0;
    int32 messageArraySize = static_cast<int32>(messageArray.size());
	
    if(!dataBuffer)
        return 0;
	
    for(i=0;i<outgoingSize;++i)
    {
        //'#bundle'
        if(i < 8)
        {
            //dataBuffer = "#bundle";
            dataBuffer[0] = '#';
            dataBuffer[1] = 'b';
            dataBuffer[2] = 'u';
            dataBuffer[3] = 'n';
            dataBuffer[4] = 'd';
            dataBuffer[5] = 'l';
            dataBuffer[6] = 'e';
            dataBuffer[7] = static_cast<char>(NULL);
            i += 7;
        }
        //timeTag
        else if(i < 16)
        {
            tempL = timeTag.getSeconds();
            tempInt = htonl(tempL);
            tempBytes = reinterpret_cast<GetTheBytes *>(&tempInt);
            dataBuffer[i] = tempBytes->a;
            ++i;
            dataBuffer[i] = tempBytes->b;
            ++i;
            dataBuffer[i] = tempBytes->c;
            ++i;
            dataBuffer[i] = tempBytes->d;
            ++i;
			
            tempL = timeTag.getFraction();
            tempInt = htonl(tempL);
            tempBytes = reinterpret_cast<GetTheBytes *>(&tempInt);
            dataBuffer[i] = tempBytes->a;
            ++i;
            dataBuffer[i] = tempBytes->b;
            ++i;
            dataBuffer[i] = tempBytes->c;
            ++i;
            dataBuffer[i] = tempBytes->d;
        }
        //Do the messages first.
        else if(messageCount < messageArraySize)
        {
            tempInt = htonl(messageArray[messageCount]->getSize());
            tempBytes = reinterpret_cast<GetTheBytes *>(&tempInt);
            dataBuffer[i] = tempBytes->a;
            ++i;
            dataBuffer[i] = tempBytes->b;
            ++i;
            dataBuffer[i] = tempBytes->c;
            ++i;
            dataBuffer[i] = tempBytes->d;
            ++i;
			
            tempBuffer = messageArray[messageCount]->getData();
            for(j=0;j<static_cast<int32>(ntohl(tempInt));++j)
                dataBuffer[i+j] = tempBuffer[j];
            i += (j-1);
			
            ++messageCount;
        }
        //Now the bundles.
        else if(bundleCount < static_cast<int32>(bundleArray.size()))
        {
            tempInt = htonl(bundleArray[bundleCount]->getSize());
            tempBytes = reinterpret_cast<GetTheBytes *>(&tempInt);
            dataBuffer[i] = tempBytes->a;
            ++i;
            dataBuffer[i] = tempBytes->b;
            ++i;
            dataBuffer[i] = tempBytes->c;
            ++i;
            dataBuffer[i] = tempBytes->d;
            ++i;
			
            tempBuffer = bundleArray[bundleCount]->getData();
            for(j=0;j<static_cast<int32>(ntohl(tempInt));++j)
                dataBuffer[i+j] = tempBuffer[j];
            i += (j-1);
			
            ++bundleCount;
        }
    }
	
    return dataBuffer;
}

//------------------------------------------------------------------------------
void OpenSoundBundle::addElement(OpenSoundBase *entry, bool deleteWhenUsed)
{
    int32 size;
	
    size = entry->getSize();
    if(isBundle(entry->getData(), size))
    {
        bundleArray.push_back(static_cast<OpenSoundBundle *>(entry));
        bundleShouldBeDeleted.push_back(deleteWhenUsed);
    }
    else if(OpenSoundMessage::isMessage(entry->getData(), size))
    {
        messageArray.push_back(static_cast<OpenSoundMessage *>(entry));
        messageShouldBeDeleted.push_back(deleteWhenUsed);
    }
}

//------------------------------------------------------------------------------
void OpenSoundBundle::addMessage(OpenSoundMessage *message, bool deleteWhenUsed)
{
    messageArray.push_back(message);
    messageShouldBeDeleted.push_back(deleteWhenUsed);
}

//------------------------------------------------------------------------------
void OpenSoundBundle::clearElements()
{
    std::vector<OpenSoundMessage *>::iterator itM;
    std::vector<OpenSoundBundle *>::iterator itB;
    std::vector<bool>::iterator itD;
	
    for(itM=messageArray.begin(), itD=messageShouldBeDeleted.begin();
        itM!=messageArray.end();
        ++itM, ++itD)
    {
        if(*itD)
            delete (*itM);
    }
    for(itB=bundleArray.begin(), itD=bundleShouldBeDeleted.begin();
        itB!=bundleArray.end();
        ++itB, ++itD)
    {
        if(*itD)
            delete (*itB);
    }
	
    messageArray.clear();
    messageShouldBeDeleted.clear();
    bundleArray.clear();
    bundleShouldBeDeleted.clear();
}

//------------------------------------------------------------------------------
void OpenSoundBundle::setTimeTag(const TimeTag& val)
{
    timeTag = val;
}

//------------------------------------------------------------------------------
long OpenSoundBundle::getNumElements() const
{
    return messageArray.size() + bundleArray.size();
}

//------------------------------------------------------------------------------
OpenSoundMessage *OpenSoundBundle::getMessage(const long index)
{
    OpenSoundMessage *retval = 0;
	
    if(index < static_cast<int32>(messageArray.size()))
        retval = messageArray[index];
	
    return retval;
}

//------------------------------------------------------------------------------
OpenSoundBundle *OpenSoundBundle::getBundle(const long index)
{
    OpenSoundBundle *retval = 0;
	
    if(index < static_cast<int32>(bundleArray.size()))
        retval = bundleArray[index];
	
    return retval;
}

//------------------------------------------------------------------------------
bool OpenSoundBundle::isBundle(char *data, const int size)
{
    bool retval = false;
	
    if(size > 8)
    {
        //Should only need to check for the '#bundle' string...
        if((data[0] == '#') &&
           (data[1] == 'b') &&
           (data[2] == 'u') &&
           (data[3] == 'n') &&
           (data[4] == 'd') &&
           (data[5] == 'l') &&
           (data[6] == 'e'))
			retval = true;
    }
	
    return retval;
}

END_JUCE_NAMESPACE

