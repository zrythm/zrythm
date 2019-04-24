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

BEGIN_JUCE_NAMESPACE

#include "jucetice_GraphNodeComponent.h"
#include "jucetice_GraphConnectorComponent.h"


//==============================================================================
GraphLinkComponent::GraphLinkComponent (const bool leftToRight_,
                                        const int lineThickness_,
                                        const float tension_)
    : from (0),
      to (0),
      startPointX (0),
      startPointY (0),
      endPointX (0),
      endPointY (0),
      leftToRight (leftToRight_),
      lineThickness (lineThickness_),
      tension (tension_)
{
    setInterceptsMouseClicks (true, false);
    setRepaintsOnMouseActivity (true);
}

GraphLinkComponent::~GraphLinkComponent()
{
    if (to) to->removeLink (this);
    if (from) from->removeLink (this);
}

//==============================================================================
void GraphLinkComponent::setStartPoint (const int x, const int y)
{
    startPointX = x;
    startPointY = y;

    updateBounds();
}

void GraphLinkComponent::setEndPoint (const int x, const int y)
{
    endPointX = x;
    endPointY = y;

    updateBounds();
}

//==============================================================================
void GraphLinkComponent::setTension (const float newTension)
{
    tension = jmin (jmax (newTension, 0.0f), 1.0f);
}

//==============================================================================
void GraphLinkComponent::mouseUp (const MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
    {
        GraphNodeComponent* node = (to ? to->getParentGraphComponent ()
                                       : (from ? from->getParentGraphComponent ()
                                               : 0));
        if (node)
            node->notifyLinkPopupMenuSelected (this);
    }
}

//==============================================================================
void GraphLinkComponent::mouseDrag (const MouseEvent& e)
{
    if (e.mods.isLeftButtonDown())
    {
        setTension (tension + e.getDistanceFromDragStartY () / 512.0f);

        updateBounds();
        repaint ();
    }
}

//==============================================================================
void GraphLinkComponent::paint (Graphics& g)
{
    Colour wireColour = Colours::black;
    if ((to != 0 && from != 0))
    {
        GraphNodeComponent* node = to->getParentGraphComponent ();
        wireColour = node->getConnectorColour (to, isMouseOverOrDragging());
    }
    else
    {
        if (to)
        {
            GraphNodeComponent* node = to->getParentGraphComponent ();
            wireColour = node->getConnectorColour (to, true);
        }
        else if (from)
        {
            GraphNodeComponent* node = from->getParentGraphComponent ();
            wireColour = node->getConnectorColour (from, true);
        }
    }

    g.setColour (wireColour);
    g.strokePath (path, PathStrokeType (lineThickness));
}

bool GraphLinkComponent::hitTest (int x, int y)
{
	Line<float> l (x, y, x + lineThickness, y + lineThickness);
    return path.intersectsLine (l);
}

//==============================================================================
void GraphLinkComponent::updateBounds()
{
    int min_x = jmin (endPointX, startPointX),
        min_y = jmin (endPointY, startPointY),
        max_x = jmax (endPointX, startPointX),
        max_y = jmax (endPointY, startPointY);

    setBounds ((int) (min_x - lineThickness),
                (int) (min_y - lineThickness),
                (int) ((max_x - min_x) + (lineThickness * 2)),
                (int) ((max_y - min_y) + (lineThickness * 2)));

    // recalculate path
    float x1 = (float) (startPointX - getX());
    float y1 = (float) (startPointY - getY());
    float x2 = (float) (endPointX - getX());
    float y2 = (float) (endPointY - getY());

    // recalculate bounds
    path.clear ();
    path.startNewSubPath (x1, y1);

    if (leftToRight)
    {
        float cx = fabs ((x2 - x1));
        path.cubicTo (cx * tension, y1,
                      cx * (1.0f - tension), y2,
                      x2, y2);
    }
    else
    {
        float cx = fabs ((y2 - y1));
        path.cubicTo (x1, cx * tension,
                      x2, cx * (1.0f - tension),
                      x2, y2);
    }
}

END_JUCE_NAMESPACE
