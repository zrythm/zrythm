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

CoordinateSystemRange::CoordinateSystemRange(double initMinX, double initMaxX,
                                             double initMinY, double initMaxY)
 {
  minX = -2.2;
  maxX = +2.2;
  minY = -2.2;
  maxY = +2.2;
 }

CoordinateSystemRange::~CoordinateSystemRange()
{

}

void CoordinateSystemRange::setMinX(double newMinX) 
{ 
 jassert( newMinX < maxX );
 if( newMinX < maxX )
  minX = newMinX;
}

void CoordinateSystemRange::setMaxX(double newMaxX) 
{ 
 jassert( newMaxX > minX );
 if( newMaxX > minX )
  maxX = newMaxX;
}

void CoordinateSystemRange::setMinY(double newMinY) 
{ 
 jassert( newMinY < maxY );
 if( newMinY < maxY )
  minY = newMinY;
}

void CoordinateSystemRange::setMaxY(double newMaxY) 
{ 
 jassert( newMaxY > minY );
 if( newMaxY > minY )
  maxY = newMaxY;
}

END_JUCE_NAMESPACE
