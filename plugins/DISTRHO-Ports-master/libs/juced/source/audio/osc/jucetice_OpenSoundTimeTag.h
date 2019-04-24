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

#ifndef __JUCETICE_OPENSOUNDTIMETAG_HEADER__
#define __JUCETICE_OPENSOUNDTIMETAG_HEADER__

//==============================================================================
///    Class representing an OSC TimeTag, used by Bundle.
class TimeTag
{
public:

    ///    Copy constructor.
    /*!
        Necessary?
     */
    TimeTag(const TimeTag& copy);

    ///    Default Constructor.
    /*!
        Initialises the time to the 'immediately' - i.e. 0x0000000000000001.
     */
    TimeTag();

    ///    Constructor to initialise the time to a particular time.
    TimeTag(const uint32 seconds, const uint32 fraction);

    ///    Constructor to initialise the time from a char block of 8 bytes.
    /*!
        Note: the data passed in must be at least 8 bytes long!

        This is most useful when you've just received a block of char data and
        need to determine it's time tag.
     */
    TimeTag(char *data);

    ///    Returns the seconds value for this time tag.
    uint32 getSeconds() const { return seconds; }

    ///    Returnes the fractional part for this time tag.
    uint32 getFraction() const { return fraction; }

    ///    Returns the time represented as a string.
    String getTimeAsText();

    ///    Sets the seconds value for this time tag.
    void setSeconds(const uint32 val);

    ///    Sets the fractional value for this time tag.
    void setFraction(const uint32 val);

    ///    Sets the seconds and fractional values for this time tag.
    void setTime(const uint32 seconds, const uint32 fraction);

    ///    Static method to create a new TimeTag with the current time.
    static TimeTag getCurrentTime();

    ///    So we can test TimeTags against each other.
    bool operator==(const TimeTag& other);

    ///    So we can test TimeTags against each other.
    bool operator!=(const TimeTag& other);

    ///    So we can test TimeTags against each other.
    bool operator<(const TimeTag& other);

    ///    So we can test TimeTags against each other.
    bool operator>(const TimeTag& other);

    ///    So we can test TimeTags against each other.
    bool operator<=(const TimeTag& other);

    ///    So we can test TimeTags against each other.
    bool operator>=(const TimeTag& other);

    ///    So we can assign one TimeTag's values to another.
    TimeTag operator=(const TimeTag& other);

private:

    uint32 seconds;
    uint32 fraction;
};

#endif
