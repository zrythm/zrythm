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

#ifndef __JUCETICE_OPENSOUNDBASE_HEADER__
#define __JUCETICE_OPENSOUNDBASE_HEADER__

//------------------------------------------------------------------------------
///    OSC's OSC-string type.
typedef char * OscString;
///    A padding byte.
typedef char PaddingByte;

//------------------------------------------------------------------------------
///    The base class for any OSC message/bundle packets.
class OpenSoundBase
{
public:
    ///    Constructor.
    OpenSoundBase() {};
    ///    Destructor.
    virtual ~OpenSoundBase() {};
	
    ///    Returns the total size of the OSC data to be sent.
    virtual int32 getSize() = 0;
    ///    Returns the data to be sent as a contiguous unsigned char array.
    virtual char* getData() = 0;
};

///    Used to easily access the individual bytes in Int32 and Float32s.
struct GetTheBytes
{
    unsigned char a;
    unsigned char b;
    unsigned char c;
    unsigned char d;
};


#endif
