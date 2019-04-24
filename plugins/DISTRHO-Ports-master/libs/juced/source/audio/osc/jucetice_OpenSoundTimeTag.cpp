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
  #include <sys/timeb.h>
  #include <time.h>
#else
  #include <netinet/in.h>
#endif

BEGIN_JUCE_NAMESPACE

//==============================================================================
TimeTag::TimeTag(const uint32 seconds, const uint32 fraction)
{
    this->seconds = seconds;
    this->fraction = fraction;
}

TimeTag::TimeTag(char *data)
{
    uint32 *tempBytes;

    //Get the seconds.
    tempBytes = reinterpret_cast<uint32 *>(data);
    seconds = ntohl(*tempBytes);

    //Get the fractional part.
    tempBytes = reinterpret_cast<uint32 *>(data+4);
    fraction = ntohl(*tempBytes);
}

TimeTag::TimeTag(const TimeTag& copy)
{
    seconds = copy.seconds;
    fraction = copy.fraction;
}

TimeTag::TimeTag()
{
    seconds = 0;
    fraction = 1;
}

//==============================================================================
String TimeTag::getTimeAsText()
{
    String out;
    out << String(seconds) << "." << (int)fraction;
    return out;
}

//==============================================================================
void TimeTag::setSeconds(const uint32 val)
{
    seconds = val;
}

void TimeTag::setFraction(const uint32 val)
{
    fraction = val;
}

void TimeTag::setTime(const uint32 seconds, const uint32 fraction)
{
    this->seconds = seconds;
    this->fraction = fraction;
}

//==============================================================================
TimeTag TimeTag::getCurrentTime()
{
    uint32 retSeconds;
    uint32 retFraction;

#ifdef WIN32
    _timeb currentTime;

    _ftime(&currentTime);

    //2208988800 = Num. seconds from 1900 to 1970, where _ftime starts from.
    retSeconds = 2208988800 + static_cast<uint32>(currentTime.time);
    //Correct for timezone.
    retSeconds -= static_cast<uint32>(60 * currentTime.timezone);
    //Correct for daylight savings time.
    if(currentTime.dstflag)
        retSeconds += static_cast<uint32>(3600);

    retFraction = static_cast<uint32>(currentTime.millitm);
    //Fill up all 32 bits...
    retFraction *= static_cast<uint32>(static_cast<float>(1<<31)/1000000.0f);
#else
    retSeconds = 0;
    retFraction = 0;
#endif

    return TimeTag(retSeconds, retFraction);
}

//==============================================================================
bool TimeTag::operator==(const TimeTag& other)
{
    bool retval = false;

    if(seconds == other.seconds)
    {
        if(fraction == other.fraction)
            retval = true;
    }

    return retval;
}

//==============================================================================
bool TimeTag::operator!=(const TimeTag& other)
{
    bool retval = false;

    if(seconds != other.seconds)
        retval = true;
    else if(fraction != other.fraction)
        retval = true;

    return retval;
}

//==============================================================================
bool TimeTag::operator<(const TimeTag& other)
{
    bool retval = false;

    if(fraction < other.fraction)
    {
        if(seconds <= other.seconds)
            retval = true;
    }
    else if(seconds < other.seconds)
        retval = true;

    return retval;
}

//==============================================================================
bool TimeTag::operator>(const TimeTag& other)
{
    bool retval = false;

    if(fraction > other.fraction)
    {
        if(seconds >= other.seconds)
            retval = true;
    }
    else if(seconds > other.seconds)
        retval = true;

    return retval;
}

//==============================================================================
bool TimeTag::operator<=(const TimeTag& other)
{
    bool retval = false;

    if(fraction <= other.fraction)
    {
        if(seconds <= other.seconds)
            retval = true;
    }
    else if(seconds <= other.seconds)
        retval = true;

    return retval;
}

//==============================================================================
bool TimeTag::operator>=(const TimeTag& other)
{
    bool retval = false;

    if(fraction > other.fraction)
    {
        if(seconds >= other.seconds)
            retval = true;
    }
    else if(seconds > other.seconds)
        retval = true;

    return retval;
}

//==============================================================================
TimeTag TimeTag::operator=(const TimeTag& other)
{
    seconds = other.seconds;
    fraction = other.fraction;

    return *this;
}

END_JUCE_NAMESPACE