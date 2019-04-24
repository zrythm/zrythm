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

#ifndef __JUCETICE_COORDINATESYSTEMRANGE_HEADER__
#define __JUCETICE_COORDINATESYSTEMRANGE_HEADER__


class CoordinateSystemRange
{
public:

 /** Standard Constructor. */
 CoordinateSystemRange(double initMinX = -2.2, double initMaxX = +2.2,
                       double initMinY = -2.2, double initMaxY = +2.2);

 /** Destructor. */
 virtual ~CoordinateSystemRange();  

 /** Returns the minimum x-value. */
 virtual double getMinX() const { return minX; }

 /** Returns the maximum x-value. */
 virtual double getMaxX() const { return maxX; }

 /** Returns the minimum y-value. */
 virtual double getMinY() const { return minY; }

 /** Returns the maximum y-value. */
 virtual double getMaxY() const { return maxY; }

 /** Sets the minimum x-value. */
 virtual void setMinX(double newMinX);

 /** Sets the maximum x-value. */
 virtual void setMaxX(double newMaxX);

 /** Sets the minimum y-value. */
 virtual void setMinY(double newMinY);

 /** Sets the maximum y-value. */
 virtual void setMaxY(double newMaxY); 

protected:

 double minX, maxX, minY, maxY;
};

#endif
