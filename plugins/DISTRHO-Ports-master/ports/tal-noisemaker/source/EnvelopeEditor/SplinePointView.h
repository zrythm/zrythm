/*
	==============================================================================
	This file is part of Tal-Reverb by Patrick Kunz.

	Copyright(c) 2005-2009 Patrick Kunz, TAL
	Togu Audio Line, Inc.
	http://kunz.corrupt.ch

	This file may be licensed under the terms of of the
	GNU General Public License Version 2 (the ``GPL'').

	Software distributed under the License is distributed
	on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
	express or implied. See the GPL for the specific language
	governing rights and limitations.

	You should have received a copy of the GPL along with this
	program. If not, go to http://www.gnu.org/licenses/gpl.html
	or write to the Free Software Foundation, Inc.,  
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
	==============================================================================
 */


#ifndef SplinePointView_H
#define SplinePointView_H

#include "PositionUtility.h"

class SplinePointView
{
private:
    static const int CENTER_SIZE = 16;

    SplinePoint *splinePoint;
    PositionUtility *pu;

public:
    SplinePointView(SplinePoint *splinePoint, PositionUtility *pu)
    {
        this->splinePoint = splinePoint;
        this->pu = pu;
    }

    void paint(Graphics& g)
    {
        this->drawCenterPoint(g);
        this->drawControlPoints(g);
    }

    void drawControlPoints(Graphics& g)
    {
        if (this->splinePoint->isSelected())
        {
            if (!this->splinePoint->IsStartPoint())
            {
                this->setControlPointColor(g, splinePoint->isControlPointSelectedLeft());
                Point<int> pointLeft = pu->calculateViewPosition(splinePoint->getControlPointLeft());
                g.fillEllipse((float)pointLeft.getX() - CENTER_SIZE / 4, (float)pointLeft.getY() - CENTER_SIZE / 4, (float)CENTER_SIZE / 2, (float)CENTER_SIZE / 2);
            }

            if (!this->splinePoint->IsEndPoint())
            {
                this->setControlPointColor(g, splinePoint->isControlPointSelectedRight());
                Point<int> pointRight = pu->calculateViewPosition(splinePoint->getControlPointRight());
                g.fillEllipse((float)pointRight.getX() - CENTER_SIZE / 4, (float)pointRight.getY() - CENTER_SIZE / 4, (float)CENTER_SIZE / 2, (float)CENTER_SIZE / 2);    
            }
        }
    }

    void setControlPointColor(Graphics& g, bool selected)
    {
        g.setColour(Colour((const uint8)170, (const uint8)170, (const uint8)170, (const uint8)180));
        if (selected)
        {
            g.setColour(Colour((const uint8)170, (const uint8)170, (const uint8)170, (const uint8)255));
        }
    }

    void drawCenterPoint(Graphics& g)
    {
        g.setColour(Colour((const uint8)100, (const uint8)100, (const uint8)100, (const uint8)100));
        if (splinePoint->isSelected())
        {
            g.setColour(Colour((const uint8)100, (const uint8)100, (const uint8)100, (const uint8)200));
        }

        Point<int> point = pu->calculateViewPosition(splinePoint->getCenterPosition());
        g.fillEllipse((float)point.getX() - CENTER_SIZE / 2, (float)point.getY() - CENTER_SIZE / 2, (float)CENTER_SIZE, (float)CENTER_SIZE);

        g.setColour(Colour((const uint8)255, (const uint8)255, (const uint8)255, (const uint8)150));
        g.drawEllipse((float)point.getX() - CENTER_SIZE / 2, (float)point.getY() - CENTER_SIZE / 2, (float)CENTER_SIZE, (float)CENTER_SIZE, 2.0f);
    }
};
#endif