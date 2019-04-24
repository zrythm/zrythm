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

#ifndef __JUCETICE_STANDARDHEADER_HEADER__
#define __JUCETICE_STANDARDHEADER_HEADER__


//==============================================================================
/** Current Jucetice version number.
 
 See also XXX::getJUCETICEVersion() for a string version.
 */
#define JUCETICE_MAJOR_VERSION      0
#define JUCETICE_MINOR_VERSION      2

/** Current Jucetice version number.
 
 Bits 16 to 32 = major version.
 Bits 8 to 16 = minor version.
 Bits 0 to 8 = point release (not currently used).
 
 See also XXX::getJUCETICEVersion() for a string version.
 */
#define JUCETICE_VERSION            ((JUCETICE_MAJOR_VERSION << 16) + (JUCETICE_MINOR_VERSION << 8))


#endif   // __JUCETICE_STANDARDHEADER_HEADER__
