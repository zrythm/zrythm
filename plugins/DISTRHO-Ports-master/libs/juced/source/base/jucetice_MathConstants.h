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

#ifndef __JUCETICE_MATHCONSTANTS_HEADER__
#define __JUCETICE_MATHCONSTANTS_HEADER__

//==============================================================================
/** A predefined value for Log2 constant, at single/double precision. */
const double  double_Log2  = 0.6931471805599453094172321214581;
const float   float_Log2   = 0.69314718055994530941f;

/** A predefined value for Log10 constant, at single/double precision. */
const double  double_Log10  = 2.302585093;
const float   float_Log10   = 2.302585093f;

/** A predefined value for Denormal, at single/double precision. */
const double  double_Denormal  = 1e-20;
const float   float_Denormal   = 1e-20;

/** A predefined value for Midi scaling, at single/double precision. */
const double  double_MidiScaler  = 1.0 / 127.0;
const float   float_MidiScaler   = 1.0f / 127.0f;

#endif
