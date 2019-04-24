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


#ifndef EnvelopeEditorView_H
#define EnvelopeEditorView_H

#include "SplinePointView.h"
#include "PositionUtility.h"

class EnvelopeEditorView : public Component, public Timer
{
private:
    static const int NUMBER_OF_VERTICAL_LINES = 4;

    PositionUtility *pu;
    TalCore* filter;

    Colour backgroundColor;
    Colour splineColor;

    float oldPhase;

public:
	EnvelopeEditorView(TalCore* const filter, int width, int height) : Component()  
	{
        this->filter = filter;
        pu = new PositionUtility(width, height);
        this->backgroundColor = Colour((const uint8)10, (const uint8)10, (const uint8)10, (const uint8)255);
        this->splineColor = Colour(Colour((const uint8)100, (const uint8)100, (const uint8)255, (const uint8)255));
        this->oldPhase = 0.0f;
        startTimer(20);
	}

    ~EnvelopeEditorView() override
    {
        stopTimer();
        deleteAllChildren();
        delete pu;
    }

    bool envelopeIsDirty()
    {
        EnvelopeEditor *envelopeEditor = this->filter->getEnvelopeEditor();
        return envelopeEditor->isDirty();
    }

    void setBackgroundColor(Colour backgroundColor)
    {
        this->backgroundColor = backgroundColor;
    }

    void setSplineColor(Colour splineColor)
    {
        this->splineColor = splineColor;
    }

    void timerCallback() override
    {
        EnvelopeEditor *envelopeEditor = this->filter->getEnvelopeEditor();
        if (this->oldPhase != envelopeEditor->getPhase() || envelopeEditor->isDirty())
        {
            this->oldPhase = envelopeEditor->getPhase();
            repaint();
        }
    }

    void paint(Graphics& g) override
    {
        EnvelopeEditor *envelopeEditor = this->filter->getEnvelopeEditor();
        this->paintBackground(g, envelopeEditor);
        this->drawCurve(g, envelopeEditor);
        this->drawPoints(g, envelopeEditor);
        this->drawPosition(g, envelopeEditor);
    }
 
    void drawPosition(Graphics& g, EnvelopeEditor *envelopeEditor)
    {
        float phase = envelopeEditor->getPhase() * (float)this->pu->getWidth();

        g.setColour(Colour((const uint8)180, (const uint8)180, (const uint8)180, (const uint8)100));
        g.drawLine(
            phase,
            0,
            phase, 
            (float)this->pu->getHeight(),
            3.0f);   
    }

    void drawCurve(Graphics& g, EnvelopeEditor *envelopeEditor)
    {
        g.setColour(this->splineColor);

        Point<float> bufferPoint(0.0f, 0.5f);
        for (int i = 0; i < this->pu->getWidth(); i += 3)
        {
            float normalizedValueX = 1.0f / this->pu->getWidth() * i;
            float normalizedValueY = envelopeEditor->getEnvelopeValue(normalizedValueX);
            Point<int> splinePoint(this->pu->calculateViewPosition(Point<float>(normalizedValueX, normalizedValueY)));

            if (i == 0)
            {
                bufferPoint.setX((float)splinePoint.getX());
                bufferPoint.setY((float)splinePoint.getY());
            }

            g.drawLine(
                (float)bufferPoint.getX(),
                (float)bufferPoint.getY(),
                (float)splinePoint.getX(), 
                (float)splinePoint.getY(),
                2.0f);
            bufferPoint.setXY((float)splinePoint.getX(), (float)splinePoint.getY());
        }
    }

    void drawPoints(Graphics& g, EnvelopeEditor *envelopeEditor)
    {
        Array<SplinePoint*> points = envelopeEditor->getPoints();

        points.getLock().enter();
        for (int i = 0; i < points.size(); i++)
        {
            SplinePoint *point = points[i];
            SplinePointView view = SplinePointView(point, this->pu);
            view.paint(g);
        }

        points.getLock().exit();
    }

    void paintBackground(Graphics& g, EnvelopeEditor *envelopeEditor)
    {
        // Background color
        g.setColour(this->backgroundColor);
        g.fillRect(0, 0, this->pu->getWidth(), this->pu->getHeight());

        this->drawBars(g, envelopeEditor);
        this->drawVerticalLines(g);

        // Border
        g.setColour(Colour((const uint8)100, (const uint8)100, (const uint8)100, (const uint8)255));
        g.drawRect(0, 0, this->pu->getWidth(), this->pu->getHeight(), 2);
    }

    void drawVerticalLines(Graphics& g)
    {
        for (int i = 1; i < NUMBER_OF_VERTICAL_LINES; i++)
        {
            if ((i % 2) == 0)
            {
                g.setColour(Colour((const uint8)230, (const uint8)230, (const uint8)230, (const uint8)120));
            }
            else
            {
                g.setColour(Colour((const uint8)120, (const uint8)120, (const uint8)120, (const uint8)120));
            }

            float normalizedValueY = 1.0f / NUMBER_OF_VERTICAL_LINES * i;
            Point<int> start = this->pu->calculateViewPosition(Point<float>(0.0f, normalizedValueY));
            Point<int> end = this->pu->calculateViewPosition(Point<float>(1.0f, normalizedValueY));
            g.drawLine((float)start.getX(), (float)start.getY(), (float)end.getX(), (float)end.getY());
        }
    }

    void drawBars(Graphics& g, EnvelopeEditor *envelopeEditor)
    {
        int numberOfBars = (int)(envelopeEditor->getNumerator() * envelopeEditor->getDenominator() / envelopeEditor->getSpeedFactor() * 2.0f);
        
        if (numberOfBars < envelopeEditor->getNumerator())
        {
            numberOfBars = (int)envelopeEditor->getNumerator();
        }
        
        for (int i = 1; i < numberOfBars; i++)
        {
            if ((i % (int)(envelopeEditor->getNumerator() * envelopeEditor->getNumerator())) == 0)
            {
                g.setColour(Colour((const uint8)250, (const uint8)250, (const uint8)250, (const uint8)150));
            }
            else if ((i % (int)envelopeEditor->getNumerator()) == 0)
            {
                g.setColour(Colour((const uint8)200, (const uint8)200, (const uint8)200, (const uint8)120));
            }
            else
            {
                g.setColour(Colour((const uint8)100, (const uint8)100, (const uint8)100, (const uint8)110));
            }

            float normalizedValueX = 1.0f / numberOfBars * i;
            Point<int> start = this->pu->calculateViewPosition(Point<float>(normalizedValueX, 0.0f));
            Point<int> end = this->pu->calculateViewPosition(Point<float>(normalizedValueX, 1.0f));
            g.drawLine((float)start.getX(), (float)start.getY(), (float)end.getX(), (float)end.getY());
        }
    }

    void deselectAllPoints()
    {
        EnvelopeEditor *envelopeEditor = this->filter->getEnvelopeEditor();
        envelopeEditor->deselect();
    }

    void mouseDown(const MouseEvent &e)
    {
        EnvelopeEditor *envelopeEditor = this->filter->getEnvelopeEditor();
        Point<float> positionNormalized = this->pu->calculateNormalizedPosition(e.getMouseDownPosition());

        if (!this->selectControlPoint(envelopeEditor, positionNormalized))
        {
            this->selectOrInsertCenterPoint(envelopeEditor, positionNormalized);
        }

        this->filter->envelopeChanged();
        this->repaint();
    }

    void selectOrInsertCenterPoint(EnvelopeEditor *envelopeEditor, Point<float> positionNormalized)
    {
        SplinePoint *selectedPoint = envelopeEditor->getSplinePoint(positionNormalized);      
        this->deselectAllPoints();

        if (selectedPoint != NULL)
        {
            selectedPoint->setSelected(true);
        }
        else
        {
            SplinePoint *newPoint = envelopeEditor->add(positionNormalized);
            newPoint->setSelected(true);
        }
    }

    bool selectControlPoint(EnvelopeEditor *envelopeEditor, Point<float> positionNormalized)
    {
        SplinePoint *alreadySelectedPoint = envelopeEditor->getSelectedSplinePoint();

        if (alreadySelectedPoint != NULL
            && (alreadySelectedPoint->isLeftControlPointSelected(positionNormalized)
            || alreadySelectedPoint->isRightControlPointSelected(positionNormalized)))
        {
            if (alreadySelectedPoint->isLeftControlPointSelected(positionNormalized))
            {
                alreadySelectedPoint->setControlPointSelectedLeft(true);
            }
            else
            {
                alreadySelectedPoint->setControlPointSelectedRight(true);
            }

            return true;
        }

        return false;
    }

    void mouseDoubleClick(const MouseEvent &e) override
    {
        EnvelopeEditor *envelopeEditor = this->filter->getEnvelopeEditor();
        SplinePoint *selectedPoint = envelopeEditor->getSelectedSplinePoint();

        if (selectedPoint != NULL
            && !selectedPoint->IsEndPoint() && !selectedPoint->IsStartPoint())
        {
            envelopeEditor->deleteSelectedSplinePoint();

            this->filter->envelopeChanged();
            this->repaint();
        }
    }

    void mouseDrag(const MouseEvent &e) override
    {
        EnvelopeEditor *envelopeEditor = this->filter->getEnvelopeEditor();
        SplinePoint *selectedPoint = envelopeEditor->getSelectedSplinePoint();

        if (selectedPoint != NULL)
        {
            Point<float> positionNormalized = this->pu->calculateNormalizedPosition(e.getPosition());

            if (!this->moveControlPoint(envelopeEditor, selectedPoint, positionNormalized))
            {
                this->moveCenterPoint(envelopeEditor, selectedPoint, positionNormalized, e);
            }

            this->filter->envelopeChanged();
            envelopeEditor->resetTimer();
            envelopeEditor->setDirty();
            this->repaint();
        }
    }

    bool moveControlPoint(
        EnvelopeEditor *envelopeEditor, 
        SplinePoint *selectedPoint, 
        Point<float> positionNormalized)
    {
        if (selectedPoint->isControlPointSelectedLeft()
            || selectedPoint->isControlPointSelectedRight())
        {
            // move control
            if (selectedPoint->isControlPointSelectedLeft())
            {
                selectedPoint->setControlPointLeftPosition(positionNormalized);
            }
            else
            {
                selectedPoint->setControlPointRightPosition(positionNormalized);
            }

            return true;
        }

        return false;
    }

    void moveCenterPoint(
        EnvelopeEditor *envelopeEditor, 
        SplinePoint *selectedPoint, 
        Point<float> positionNormalized, 
        const MouseEvent &e)
    {
        if (e.getPosition().getY() > this->pu->getHeight() + 20
            && !selectedPoint->IsEndPoint() && !selectedPoint->IsStartPoint())
        {
            envelopeEditor->deleteSelectedSplinePoint();
        }
        else
        {
            selectedPoint->setPosition(positionNormalized);
            selectedPoint->setLinkedPointPosition(positionNormalized, !envelopeEditor->isOneShot());
            envelopeEditor->sort();
        }
    }

    void mouseExit(const MouseEvent &e) override
    {
        EnvelopeEditor *envelopeEditor = this->filter->getEnvelopeEditor();
        envelopeEditor->deselect();
    }
};
#endif