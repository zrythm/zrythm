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


#ifndef EnvelopePresetUtility_H
#define EnvelopePresetUtility_H

class EnvelopePresetUtility
{
private:

public:
    EnvelopePresetUtility()
    {
    }

    void addEnvelopeDataToXml(Array<SplinePoint*> splinePoints, XmlElement* program)
    {
        XmlElement* splinePointsXml = new XmlElement ("splinePoints");
        for (int i = 0; i < splinePoints.size(); i++)
        {
            XmlElement *splinePoint = new XmlElement ("splinePoint");
            splinePoint->setAttribute(T("isStartPoint"), splinePoints[i]->IsStartPoint());
            splinePoint->setAttribute(T("isEndPoint"), splinePoints[i]->IsEndPoint());
            splinePoint->setAttribute(T("centerPointX"), splinePoints[i]->getCenterPosition().getX());
            splinePoint->setAttribute(T("centerPointY"), splinePoints[i]->getCenterPosition().getY());
            splinePoint->setAttribute(T("controlPointLeftX"), splinePoints[i]->getControlPointLeft().getX());
            splinePoint->setAttribute(T("controlPointLeftY"), splinePoints[i]->getControlPointLeft().getY());
            splinePoint->setAttribute(T("controlPointRightX"), splinePoints[i]->getControlPointRight().getX());
            splinePoint->setAttribute(T("controlPointRightY"), splinePoints[i]->getControlPointRight().getY());
            splinePointsXml->addChildElement(splinePoint);
        }
        program->addChildElement(splinePointsXml);
    }

    Array<SplinePoint*> getEnvelopeFromXml(XmlElement* program)
    {
        Array<SplinePoint*> splinePoints;

        XmlElement* xmlSplinePoints = program->getChildByName(T("splinePoints"));

        if (xmlSplinePoints != 0)
        {
            forEachXmlChildElement(*xmlSplinePoints, e)
            {
                float centerX = (float)e->getDoubleAttribute(T("centerPointX"), 0.0);
                float centerY = (float)e->getDoubleAttribute(T("centerPointY"), 0.0);
                Point<float> centerPoint(centerX, centerY);
                SplinePoint *splinePoint = new SplinePoint(centerPoint);

                centerX = (float)e->getDoubleAttribute(T("controlPointLeftX"), 0.0);
                centerY = (float)e->getDoubleAttribute(T("controlPointLeftY"), 0.0);
                Point<float> controlPointLeft(centerX, centerY);
                splinePoint->setControlPointLeftPosition(controlPointLeft);

                centerX = (float)e->getDoubleAttribute(T("controlPointRightX"), 0.0);
                centerY = (float)e->getDoubleAttribute(T("controlPointRightY"), 0.0);
                Point<float> controlPointRight(centerX, centerY);
                splinePoint->setControlPointRightPosition(controlPointRight);

                bool isStartPoint = e->getBoolAttribute(T("isStartPoint"), false);
                bool isEndPoint = e->getBoolAttribute(T("isEndPoint"), false);
                splinePoint->setStartPoint(isStartPoint);
                splinePoint->setEndPoint(isEndPoint);

                splinePoints.add(splinePoint);
            }
        }
        else
        {
            // set default points
            Point<float> centerPointStart(0.0f, 0.5f);
            SplinePoint *splinePointStart = new SplinePoint(centerPointStart);
            splinePointStart->setStartPoint(true);
            splinePoints.add(splinePointStart);

            Point<float> centerPointEnd(1.0f, 0.5f);
            SplinePoint *splinePointEnd = new SplinePoint(centerPointEnd);
            splinePointEnd->setEndPoint(true);
            splinePoints.add(splinePointEnd);
        }

        if (splinePoints.size() >= 2)
        {
            // Link start and end point
            splinePoints.getFirst()->setLinkedPoint(splinePoints.getLast());
            splinePoints.getLast()->setLinkedPoint(splinePoints.getFirst());
        }

        return splinePoints;
    }
};
#endif