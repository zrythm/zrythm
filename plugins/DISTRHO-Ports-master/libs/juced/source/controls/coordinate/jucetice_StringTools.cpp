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

BEGIN_JUCE_NAMESPACE

String decibelsToStringWithUnit1(double value)
{
 return String(value, 1) + String(" dB");
}

String decibelsToStringWithUnit2(double value)
{
 return String(value, 2) + String(" dB");
}

String hertzToStringWithUnitTotal5(double value)
{
 if( value >= 10000.0 )
  return String( (int) round(value) ) + String(" Hz");
 else if( value >= 1000.0 )
  return String(value, 1) + String(" Hz");
 else if( value >= 100.0 )
  return String(value, 2) + String(" Hz");
 else if( value >= 10.0 )
  return String(value, 3) + String(" Hz");
 else if( value >= 1.0 )
  return String(value, 4) + String(" Hz");
 else
  return String(value, 5) + String(" Hz");
}

String midiNoteToString(double midiNoteNumber)
{
 int intNoteNumber = (int) midiNoteNumber;
 switch( intNoteNumber )
 {
 case   0: return String("C-1");
 case   1: return String("C#-1");
 case   2: return String("D-1");
 case   3: return String("D#-1");
 case   4: return String("E-1");
 case   5: return String("F-1");
 case   6: return String("F#-1");
 case   7: return String("G-1");
 case   8: return String("G#-1");
 case   9: return String("A-1");
 case  10: return String("A#-1");
 case  11: return String("B-1");

 case  12: return String("C0");
 case  13: return String("C#0");
 case  14: return String("D0");
 case  15: return String("D#0");
 case  16: return String("E0");
 case  17: return String("F0");
 case  18: return String("F#0");
 case  19: return String("G0");
 case  20: return String("G#0");
 case  21: return String("A0");
 case  22: return String("A#0");
 case  23: return String("B0");

 case  24: return String("C1");
 case  25: return String("C#1");
 case  26: return String("D1");
 case  27: return String("D#1");
 case  28: return String("E1");
 case  29: return String("F1");
 case  30: return String("F#1");
 case  31: return String("G1");
 case  32: return String("G#1");
 case  33: return String("A1");
 case  34: return String("A#1");
 case  35: return String("B1");

 case  36: return String("C2");
 case  37: return String("C#2");
 case  38: return String("D2");
 case  39: return String("D#2");
 case  40: return String("E2");
 case  41: return String("F2");
 case  42: return String("F#2");
 case  43: return String("G2");
 case  44: return String("G#2");
 case  45: return String("A2");
 case  46: return String("A#2");
 case  47: return String("B2");

 case  48: return String("C3");
 case  49: return String("C#3");
 case  50: return String("D3");
 case  51: return String("D#3");
 case  52: return String("E3");
 case  53: return String("F3");
 case  54: return String("F#3");
 case  55: return String("G3");
 case  56: return String("G#3");
 case  57: return String("A3");
 case  58: return String("A#3");
 case  59: return String("B3");

 case  60: return String("C4");
 case  61: return String("C#4");
 case  62: return String("D4");
 case  63: return String("D#4");
 case  64: return String("E4");
 case  65: return String("F4");
 case  66: return String("F#4");
 case  67: return String("G4");
 case  68: return String("G#4");
 case  69: return String("A4");
 case  70: return String("A#4");
 case  71: return String("B4");

 case  72: return String("C5");
 case  73: return String("C#5");
 case  74: return String("D5");
 case  75: return String("D#5");
 case  76: return String("E5");
 case  77: return String("F5");
 case  78: return String("F#5");
 case  79: return String("G5");
 case  80: return String("G#5");
 case  81: return String("A5");
 case  82: return String("A#5");
 case  83: return String("B5");

 case  84: return String("C6");
 case  85: return String("C#6");
 case  86: return String("D6");
 case  87: return String("D#6");
 case  88: return String("E6");
 case  89: return String("F6");
 case  90: return String("F#6");
 case  91: return String("G6");
 case  92: return String("G#6");
 case  93: return String("A6");
 case  94: return String("A#6");
 case  95: return String("B6");

 case  96: return String("C7");
 case  97: return String("C#7");
 case  98: return String("D7");
 case  99: return String("D#7");
 case 100: return String("E7");
 case 101: return String("F7");
 case 102: return String("F#7");
 case 103: return String("G7");
 case 104: return String("G#7");
 case 105: return String("A7");
 case 106: return String("A#7");
 case 107: return String("B7");

 case 108: return String("C8");
 case 109: return String("C#8");
 case 110: return String("D8");
 case 111: return String("D#8");
 case 112: return String("E8");
 case 113: return String("F8");
 case 114: return String("F#8");
 case 115: return String("G8");
 case 116: return String("G#8");
 case 117: return String("A8");
 case 118: return String("A#8");
 case 119: return String("B8");

 case 120: return String("C9");
 case 121: return String("C#9");
 case 122: return String("D9");
 case 123: return String("D#9");
 case 124: return String("E9");
 case 125: return String("F9");
 case 126: return String("F#9");
 case 127: return String("G9");

 default: return String("?");
 }
}

String percentToStringWithUnit0(double value)
{
 return String( (int) round(value) ) + String(" %");
}

String percentToStringWithUnit1(double value)
{
 return String(value, 1) + String(" %");
}

String ratioToString0(double value)
{
 double percentage1 = round(100.0*(1.0-value));
 double percentage2 = round(100.0*value);
 return 
  String((int) percentage1 ) + String("/") + String((int) percentage2 );
}

String secondsToStringWithUnit3(double value)
{
 return String(value, 3) + String(" s");
}

String secondsToStringWithUnit4(double value)
{
 return String(value, 4) + String(" s");
}

String secondsToStringWithUnitTotal4(double value)
{
 if( value >= 100.0 )
  return String(value, 2) + String(" s");
 else if( value >= 1.0 )
  return String(value, 3) + String(" s");
 else if( value >= 0.1 )
  return String(1000*value, 1) + String(" ms");
 else if( value >= 0.01 )
  return String(1000*value, 2) + String(" ms");
 else if( value >= 0.001 )
  return String(1000*value, 3) + String(" ms");
 else if( value >= 0.0001 )
  return String(1000*value, 3) + String(" ms");
 else
  return String(1000*value, 3) + String(" ms"); 
}

String semitonesToStringWithUnit2(double value)
{
 return String(value, 2) + String(" st");
}



String valueToStringWithTotalNumDigits(double value, int totalNumDigits, 
                                       const String& suffix)
{
 if( totalNumDigits <= 0 )
  return String( (int) round(value) ) + suffix;
 else
 {
  if( value >= pow(10.0, totalNumDigits) )
   return String( (int) round(value) ) + suffix;
  else
  {
   int tmp                  = (int) floor(log10(fabs(value)+0.00000001));
   int numDigitsBeforePoint = jmax(1, tmp+1);
   int numDigitsAfterPoint  = totalNumDigits-numDigitsBeforePoint;
   numDigitsAfterPoint      = jmax(0, numDigitsAfterPoint);
   if( numDigitsAfterPoint == 0 )
    return String( (int) round(value) ) + suffix;
   else
    return String(value, numDigitsAfterPoint) + suffix;
  }
 }
}

String valueToString0(double value)
{
 return String( (int) round(value) ) ;
}

String valueToString1(double value)
{
 return String(value, 1);
}

String valueToString2(double value)
{
 return String(value, 2);
}

String valueToString3(double value)
{
 return String(value, 3);
}

String valueToString4(double value)
{
 return String(value, 4);
}

String valueToStringTotal5(double value)
{
 if( value >= 10000.0 )
  return String( (int) round(value) );
 else if( value >= 1000.0 )
  return String(value, 1);
 else if( value >= 100.0 )
  return String(value, 2);
 else if( value >= 10.0 )
  return String(value, 3);
 else if( value >= 1.0 )
  return String(value, 4);
 else
  return String(value, 5);
}

END_JUCE_NAMESPACE
