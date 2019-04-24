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
 
 Original Code by: braindoc
 
 ==============================================================================
*/

#ifndef __JUCETICE_STRINGTOOLS_HEADER__
#define __JUCETICE_STRINGTOOLS_HEADER__

#include "jucetice_HelperFunctions.h"


String midiNoteToString(double midiNoteNumber);
/**< Converts a midi-note number into a juce-string.  */

String decibelsToStringWithUnit1(double value);
/**< Converts a decibel-value into a juce-string with 1 digit
 after the point and adds the unit-suffix "dB".  */

String decibelsToStringWithUnit2(double value);
/**< Converts a decibel-value into a juce-string with 2 digits
 after the point and adds the unit-suffix "dB".  */

String hertzToStringWithUnitTotal5(double value);
/**< Converts a decibel-value into a juce-string with 5 digits in total
 adds the unit-suffix "Hz".  */

String percentToStringWithUnit0(double value);
/**< Converts a percent-value into a juce-string without decimal digits
 after the point and adds the unit-suffix "%".  */

String percentToStringWithUnit1(double value);
/**< Converts a percent-value into a juce-string with one decimal digit
 after the point and adds the unit-suffix "%".  */

String ratioToString0(double value);
/**< Converts a ratio into a juce-string without decimal digits. */

String semitonesToStringWithUnit2(double value);
/**< Converts a semitone-value into a juce-string with 2 digits
 after the point and adds the unit-suffix "st".  */

String secondsToStringWithUnit3(double value);
/**< Converts a time-value in seconds into a juce-string with 3 digits
 after the point and adds the unit-suffix "s".  */

String secondsToStringWithUnit4(double value);
/**< Converts a time-value in seconds into a juce-string with 4 digits
 after the point and adds the unit-suffix "s".  */

String secondsToStringWithUnitTotal4(double value);
/**< Converts a time-value in seconds into a juce-string with  a total number
 of 4 digits the unit-suffix "s" or "ms".  */

String valueToStringWithTotalNumDigits(double value, int totalNumDigits = 3, 
                                       const String& suffix = String());
/**< Converts a value into a juce-string with adjustable total number of 
 decimal digits and an optional suffix.  */

String valueToString0(double value);
/**< Converts a value into a juce-string without decimal digits after the 
 point.  */

String valueToString1(double value);
/**< Converts a value into a juce-string with 1 decimal digit after the 
 point.  */

String valueToString2(double value);
/**< Converts a value into a juce-string with 2 decimal digits after the 
 point.  */

String valueToString3(double value);
/**< Converts a value into a juce-string with 2 decimal digits after the 
 point.  */

String valueToString4(double value);
/**< Converts a value into a juce-string with 2 decimal digits after the 
 point.  */

String valueToStringTotal5(double value);
/**< Converts a value into a juce-string with 5 decimal digits in total.  */

#endif   // __JUCE_STRINGTOOLS_JUCEHEADER__

