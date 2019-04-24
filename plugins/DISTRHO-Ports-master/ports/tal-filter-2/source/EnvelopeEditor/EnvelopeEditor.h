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


#ifndef EnvelopeEditor_H
#define EnvelopeEditor_H

#include "SplinePoint.h"
#include "SplineUtility.h"

class EnvelopeEditor : Timer
{
private:
    static const int TIMER_INTERVALL = 1000;
    static const int FIX_TEMPO = 120;

    Array<SplinePoint*> points;
    Array<float> splineDataBuffer;

    SplineUtility su;
    float sampleRate;
    float samplesPerBeat;

    float actualPhase;
    float speedFactor;

    float denominator;
    float numerator;

    float bpm;

    bool dirty;

    bool oneShot;
    bool fixTempo;

    bool isPlaying;

public:
    EnvelopeEditor(float sampleRate)
    {
        this->sampleRate = sampleRate;
        this->denominator = 4;
        this->numerator = 4;
        this->dirty = true;

        this->oneShot = false;
        this->fixTempo = false;

        // default 120 bpm
        this->samplesPerBeat = sampleRate / 2.0f;
        this->speedFactor = 1.0f;
        this->actualPhase = 0.0f;
        this->bpm = FIX_TEMPO;
        this->isPlaying = true;

        this->initializePoints();

        startTimer(TIMER_INTERVALL);
    }

    ~EnvelopeEditor() override
    {
        stopTimer();
        this->splineDataBuffer.clear();
        this->points.clear();
    }

private:

    float getBufferedValue(float x)
    {
        float position = x * (splineDataBuffer.size() - 1);
        int positionInt0 = (int)position;
        
        if (positionInt0 >= splineDataBuffer.size())
        {
            positionInt0 -= splineDataBuffer.size();
        }

        float f0 = splineDataBuffer[positionInt0];

        int positionInt1 = positionInt0 + 1;
        if (positionInt1 >= splineDataBuffer.size())
        {
            positionInt1 -= splineDataBuffer.size();
        }

        float f1 = splineDataBuffer[positionInt1];

        float positionFraq = position - positionInt0;
        return f0 + (f1 - f0) * positionFraq;
    }

    CriticalSection myCriticalSectionBuffer;

    float getEnvleopeValueCalculated(float x)
    {
        for (int i = 0; i < this->points.size() - 1; i++)
        {
            if (x >= points[i]->getX() && x <= points[i + 1]->getX())
            {
                float deltaX = points[i + 1]->getX() - points[i]->getX();

                // Havent a better solution for 0 values until now
                if (deltaX == 0.0f)
                {
                    deltaX = 0.00000000001f;
                }

                float scaleX = (x - points[i]->getCenterPosition().getX()) * 1.0f / deltaX;

                Point<float> result = su.PointOnCubicBezier(
                    scaleX, 
                    points[i]->getCenterPosition(), 
                    points[i]->getControlPointRight(), 
                    points[i + 1]->getControlPointLeft(),
                    points[i + 1]->getCenterPosition()); 

                if (result.getY() > 1.0f) result.setY(1.0f);
                if (result.getY() < 0.0f) result.setY(0.0f);

                return result.getY();
            }
        }

        return 0.0f;
    }

    void calculateBuffer()
    {
        this->splineDataBuffer.clear();

        float x = 0.0f;
        float phaseDelta = this->getPhaseDelta();
        int arraySize = (int)(1.0f / phaseDelta);

        for (int i = 0; i < arraySize; i++)
        {
            x = i * phaseDelta;
            this->splineDataBuffer.add(this->getEnvleopeValueCalculated(x));
        }
        
        this->dirty = false;
    }

public:

    void initializePoints()
    {
        //const ScopedLock myScopedLock (myCriticalSectionBuffer);
        points.clear();

        SplinePoint *start = new SplinePoint(Point<float>(0.0f, 0.5f));
        SplinePoint *end = new SplinePoint(Point<float>(1.0f, 0.5f));

        start->setStartPoint(true);
        end->setEndPoint(true);

        start->setLinkedPoint(end);
        end->setLinkedPoint(start);

        points.add(start);
        points.add(end);

        this->sort();
    }

    void setOneShot(bool value)
    {
        this->oneShot = value;
    }

    bool isOneShot()
    {
        return this->oneShot;
    }

    void setFixTempo(bool value)
    {
        this->fixTempo = value;
        this->bpm = FIX_TEMPO;
    }

    float getDenominator()
    {
        return this->denominator;
    }


    float getNumerator()
    {
        return this->numerator;
    }

    float getSpeedFactor()
    {
        return this->speedFactor;
    }

    bool isDirty()
    {
        return this->dirty;
    }

    void setDirty()
    {
        this->dirty = true;
    }

    void resetTimer()
    {
        this->startTimer(TIMER_INTERVALL);
    }

    void timerCallback() override
    {
        if (this->dirty)
        {
            const ScopedLock myScopedLock (myCriticalSectionBuffer);
            this->calculateBuffer();
        }
        
        startTimer(TIMER_INTERVALL);
    }

    float getEnvelopeValue(float x)
    {
        const ScopedLock myScopedLock (myCriticalSectionBuffer);
        if (this->dirty)
        {
            return this->getEnvleopeValueCalculated(x);
        }
        else
        {
            return this->getBufferedValue(x);
        }
    }

    SplinePoint* getSplinePoint(Point<float> positionNormalized)
    {
        for (int i = 0; i < this->points.size(); i++)
        {
            if (points[i]->isSelected(positionNormalized))
            {
                return points[i];
            }
        }

        return NULL;
    }

    SplinePoint* getSelectedSplinePoint()
    {
        for (int i = 0; i < this->points.size(); i++)
        {
            if (points[i]->isSelected())
            {
                return points[i];
            }
        }

        return NULL;
    }

    Array<SplinePoint*> getPoints()
    {
        return this->points;
    }

    void setPoints(Array<SplinePoint*> splinePoints)
    {
        if (splinePoints.size() > 0)
        {
            const ScopedLock myScopedLock (myCriticalSectionBuffer);
            this->points.clear();
            this->points = splinePoints;
        }

        this->setDirty();
        deselect();
    }

    void deselect()
    {
        for (int i = 0; i < this->points.size(); i++)
        {
            points[i]->setSelected(false);
            points[i]->setControlPointSelectedLeft(false);
            points[i]->setControlPointSelectedRight(false);
        }
    }

    SplinePoint* add(Point<float> positionNormalized)
    {
        SplinePoint *newPoint = new SplinePoint(positionNormalized);
        points.insert(1, newPoint);

        this->sort();
        this->setDirty();
        return newPoint;
    }

    void sort()
    {
        SplinePointComparator comarator;
        points.sort(comarator, true);
    }

    bool deleteSelectedSplinePoint()
    {
        SplinePoint* selected = this->getSelectedSplinePoint();

        for (int i = 0; i < this->points.size(); i++)
        {
            if (points[i] == selected
                && points[i]->canDelete())
            {
                SplinePoint *removedPoint = points.removeAndReturn(i);
                delete removedPoint;
                this->sort();
                this->setDirty();
                return true;
            }
        }

        return false;
    }

    float getPhase()
    {
        return this->actualPhase;
    }

    // returns a float [0..1]
    float process()
    {
        if (this->isPlaying)
        {
            this->actualPhase += this->getPhaseDelta(); 
            if (this->actualPhase >= 1.0f)
            {
                this->actualPhase -= 1.0f;
            }
        }

        return this->getEnvelopeValue(this->actualPhase);
    }

    void sync(float bpm, AudioPlayHead::CurrentPositionInfo pos)
    {
        this->setTimeInformation(pos);
        this->actualPhase = speedFactor / this->numerator * ((float)pos.ppqPosition / this->denominator);

        // We need this to avoid drop outs
        this->actualPhase = this->actualPhase - floorf(this->actualPhase);
    }

    void setTimeInformation(AudioPlayHead::CurrentPositionInfo pos)
    {
        this->isPlaying = pos.isPlaying;

        if (this->bpm != (float)pos.bpm
            || this->denominator != (float)pos.timeSigDenominator
            || this->numerator != (float)pos.timeSigNumerator)
        {
            this->setTimeInformation((float)pos.bpm);
            this->denominator = (float)pos.timeSigDenominator;
            this->numerator = (float)pos.timeSigNumerator;

            if (this->denominator == 0.0f)
            {
                this->denominator = 4.0f;
            }

            if (this->numerator == 0.0f)
            {
                this->numerator = 4.0f;
            }

            this->setDirty();
        }
    }

    void setTimeInformation(float bpm)
    {
        if (this->bpm > 0.0f)
        {
            this->bpm  = bpm;
            this->samplesPerBeat = getSamplesPerBeat(bpm);
        }
    }

    float getSamplesPerBeat(float bpm)
    {
        if (this->fixTempo)
        {
            return 60.0f / FIX_TEMPO * sampleRate;
        }

	    return 60.0f / bpm * sampleRate;
    }

    float getPhaseDelta() 
    {
        return 1.0f / (this->samplesPerBeat * this->denominator) * speedFactor / this->numerator;
    }

    void setSpeedFactor(int index)
    {
        switch(index)
        {
            case 1: this->speedFactor = 1.0f; break;
            case 2: this->speedFactor = 2.0f; break;
            case 3: this->speedFactor = 4.0f; break;
            case 4: this->speedFactor = 8.0f; break;
            case 5: this->speedFactor = 16.0f; break;
            case 6: this->speedFactor = 32.0f; break;
            default: this->speedFactor = 1.0f;
        }

        this->setDirty();
    }
};
#endif