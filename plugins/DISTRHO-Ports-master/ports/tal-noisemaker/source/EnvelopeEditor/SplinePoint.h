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


#ifndef SplinePoint_H
#define SplinePoint_H

#include "math.h"

class SplinePoint
{
private:
    const float SELECTION_RADIUS;

    Point<float> centerPosition;

    Point<float> controlPointLeft;
    Point<float> controlPointRight;

    bool isStartPoint;
    bool isEndPoint;

    bool selected;
    bool controlPointSelectedLeft;
    bool controlPointSelectedRight;

    SplinePoint *linkedPoint;

public:
    SplinePoint(Point<float> centerPosition) : SELECTION_RADIUS(1.0f / 24.0f)
    {
        this->isStartPoint = false;
        this->isEndPoint = false;
        this->selected = false;
        this->controlPointSelectedLeft = false;
        this->controlPointSelectedRight = false;
        this->linkedPoint = NULL;

        // set default control values
        controlPointLeft.setXY(centerPosition.getX() - 0.1f, centerPosition.getY());
        controlPointRight.setXY(centerPosition.getX() + 0.1f, centerPosition.getY());
        this->centerPosition.setXY(centerPosition.getX(), centerPosition.getY());
        
        this->setPosition(centerPosition);
    }

    ~SplinePoint()
    {
    }

    bool isSelected()
    {
        return selected;
    }

    bool isControlPointSelectedLeft()
    {
        return this->controlPointSelectedLeft;
    }

    bool isControlPointSelectedRight()
    {
        return this->controlPointSelectedRight;
    }

    void setControlPointSelectedLeft(bool selected)
    {
        this->controlPointSelectedLeft = selected;
        this->controlPointSelectedRight = false;
    }

    void setControlPointSelectedRight(bool selected)
    {
        this->controlPointSelectedRight = selected;
        this->controlPointSelectedLeft = false;
    }

    void setSelected(bool selected)
    {
        this->selected = selected;

        this->controlPointSelectedLeft = false;
        this->controlPointSelectedRight = false;
    }

    bool IsStartPoint()
    {
        return isStartPoint;
    }


    bool IsEndPoint()
    {
        return isEndPoint;
    }

    void setStartPoint(bool isStartPoint)
    {
        this->isStartPoint = isStartPoint;
    }

    void setEndPoint(bool isEndPoint)
    {
        this->isEndPoint = isEndPoint;
    }

    void setLinkedPoint(SplinePoint *linkedPoint)
    {
        this->linkedPoint = linkedPoint;
    }

    Point<float> getCenterPosition()
    {
        return this->centerPosition;
    }

    Point<float> getControlPointLeft()
    {
        return this->controlPointLeft;
    }

    Point<float> getControlPointRight()
    {
        return this->controlPointRight;
    }

    float getX()
    {
        return this->centerPosition.getX();
    }

    float getY()
    {
        return this->centerPosition.getY();
    }

    void setPosition(Point<float> position)
    {
        position = this->limitBoundaries(position);

        Point<float> deltaPointLeft = this->centerPosition - this->controlPointLeft;
        Point<float> deltaPointRight = this->centerPosition - this->controlPointRight;

        // start and end points cant move in x
        if (!this->isStartPoint && !this->isEndPoint)
        {
            this->centerPosition.setX(position.getX());
        }

        this->centerPosition.setY(position.getY());

        // calculate new control point positions
        this->controlPointLeft.setXY(this->centerPosition.getX(), this->centerPosition.getY());
        this->controlPointRight.setXY(this->centerPosition.getX(), this->centerPosition.getY());

        this->controlPointLeft -= deltaPointLeft;
        this->controlPointRight -= deltaPointRight;

        this->controlPointLeft = this->limitBoundaries(this->controlPointLeft);
        this->controlPointRight = this->limitBoundaries(this->controlPointRight);
    }

    void setLinkedPointPosition(Point<float> position, bool doLinkEndpoints)
    {
        if (this->linkedPoint != NULL && doLinkEndpoints)
        {
            this->linkedPoint->setPosition(position);
        }
    }

    void setControlPointLeftPosition(Point<float> position)
    {
        position = this->limitBoundaries(position);
        this->controlPointLeft.setXY(position.getX(), position.getY());

        if (this->controlPointLeft.getX() > this->centerPosition.getX())
        {
            this->controlPointLeft.setX(this->centerPosition.getX());
        }
    }

    void setControlPointRightPosition(Point<float> position)
    {
        position = this->limitBoundaries(position);
        this->controlPointRight.setXY(position.getX(), position.getY());

        if (this->controlPointRight.getX() < this->centerPosition.getX())
        {
            this->controlPointRight.setX(this->centerPosition.getX());
        }
    }

    Point<float> limitBoundaries(Point<float> position)
    {
        if (position.getX() < 0.0f) position.setX(0.0f);
        if (position.getX() > 1.0f) position.setX(1.0f);
        if (position.getY() < 0.0f) position.setY(0.0f);
        if (position.getY() > 1.0f) position.setY(1.0f);

        return position;
    }

    bool isSelected(Point<float> positionNormalized)
    {
        float deltaX = positionNormalized.getX() - this->centerPosition.getX();
        float deltaY = positionNormalized.getY() - this->centerPosition.getY();

        return this->verifyDistance(deltaX, deltaY, SELECTION_RADIUS);
    }

    bool isLeftControlPointSelected(Point<float> positionNormalized)
    {
        if (this->isSelected() && !this->IsStartPoint())
        {
            float deltaX = positionNormalized.getX() - this->controlPointLeft.getX();
            float deltaY = positionNormalized.getY() - this->controlPointLeft.getY();
            
            return this->verifyDistance(deltaX, deltaY, SELECTION_RADIUS);
        }

        return false;
    }

    bool isRightControlPointSelected(Point<float> positionNormalized)
    {
        if (this->isSelected() && !this->IsEndPoint())
        {
            float deltaX = positionNormalized.getX() - this->controlPointRight.getX();
            float deltaY = positionNormalized.getY() - this->controlPointRight.getY();

            return this->verifyDistance(deltaX, deltaY, SELECTION_RADIUS);
        }

        return false;
    }

    bool verifyDistance(float deltaX, float deltaY, float radius)
    {
        float distance = sqrtf(deltaX * deltaX + deltaY * deltaY);

        if (distance <= radius)
        {
            return true;
        }
        return false;
    }

    bool canDelete()
    {
        return !this->IsStartPoint() && !this->IsEndPoint();
    }
};

// This will sort a set of components, so that they are ordered in terms of
// left-to-right and then top-to-bottom.
class SplinePointComparator
{
public:
	SplinePointComparator() {}

    static int compareElements(SplinePoint* first, SplinePoint* second)
    {
        float firstValueX = first->getX();
        float secondValueX = second->getX();

        if (first->IsStartPoint())
        {
            return -1;
        }

        if (first->IsEndPoint())
        {
            return 1;
        }

        if (second->IsStartPoint())
        {
            return 1;
        }

        if (second->IsEndPoint())
        {
            return -1;
        }

        if (firstValueX > secondValueX)
        {
            return 1;
        }
        else
        {
            if (firstValueX != secondValueX)
            {
                return -1;
            }
        }

        return 0;
    }
};
#endif