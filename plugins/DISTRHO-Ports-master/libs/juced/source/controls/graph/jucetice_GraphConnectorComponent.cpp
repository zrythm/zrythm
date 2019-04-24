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

#include "jucetice_GraphLinkComponent.h"
#include "jucetice_GraphNodeComponent.h"

//==============================================================================
GraphConnectorComponent::GraphConnectorComponent (GraphNodeComponent* parentGraph_,
                                                  const int connectorType_)
    : parentGraph (parentGraph_),
      relativePositionX (0),
      relativePositionY (0),
      newLink (0),
      currentlyDraggingOver (0),
      connectorID (0),
      connectorType (connectorType_),
      connectionDraggedOver (false)
{
    leftToRight = parentGraph->isLeftToRight ();

    setSize (16, 16);
    updatePosition();
    setRepaintsOnMouseActivity (true);
}

GraphConnectorComponent::~GraphConnectorComponent()
{
}

//==============================================================================
void GraphConnectorComponent::paint (Graphics& g)
{
#if 0
    if (isMouseOverOrDragging() || connectionDraggedOver)
    {
        g.fillAll (parentGraph->getConnectorColour (this, true));
    }
    else
    {
        g.fillAll (parentGraph->getConnectorColour (this, false));
    }
#endif

    const float w = (float) getWidth();
    const float h = (float) getHeight();
    
    if (isMouseOverOrDragging() || connectionDraggedOver)
        g.setColour (parentGraph->getConnectorColour (this, true));
    else
        g.setColour (parentGraph->getConnectorColour (this, false));

    Path p;
    if (leftToRight)
    {
        p.addEllipse (w * 0.25f, h * 0.25f, w * 0.5f, h * 0.5f);
        p.addRectangle (isInput() ? (0.5f * w) : 0.0f, h * 0.4f, w * 0.5f, h * 0.2f);
    }
    else
    {
        p.addEllipse (w * 0.25f, h * 0.25f, w * 0.5f, h * 0.5f);
        p.addRectangle (w * 0.4f, isInput() ? (0.5f * h) : 0.0f, w * 0.2f, h * 0.5f);
    }

    g.fillPath (p);
}

//==============================================================================
void GraphConnectorComponent::mouseDown (const MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
    {
        if (parentGraph)
            parentGraph->notifyConnectorPopupMenuSelected (this);
    }
}

//==============================================================================
void GraphConnectorComponent::connectionDragEnter()
{
    connectionDraggedOver = true;
    repaint();
}

//==============================================================================
void GraphConnectorComponent::connectionDragExit()
{
    connectionDraggedOver = false;
    repaint();
}

//==============================================================================
void GraphConnectorComponent::setRelativePosition (const int x, const int y)
{
    relativePositionX = x;
    relativePositionY = y;

    updatePosition();
}

//==============================================================================
void GraphConnectorComponent::updatePosition()
{
    int x = parentGraph->getX() + relativePositionX;
    int y = parentGraph->getY() + relativePositionY;

    setBounds (x, y, getWidth(), getHeight());
}

//==============================================================================
void GraphConnectorComponent::addLink (GraphLinkComponent* newLink)
{
    links.add (newLink);

    if (parentGraph)
        parentGraph->notifyLinkConnected (newLink);
}

//==============================================================================
void GraphConnectorComponent::removeLink (GraphLinkComponent* linkToDelete)
{
    links.removeObject (linkToDelete);
}

//==============================================================================
void GraphConnectorComponent::removeLinkWith (GraphConnectorComponent* connector, const bool notifyListeners)
{
DBG ("START >>> GraphConnectorComponent::removeLinkWith (GraphConnectorComponent* connector)");

    for (int i = 0; i < links.size (); i++)
    {
		if( links[i]->from == connector || links[i]->to == connector )
        {
            if (notifyListeners && parentGraph)
                parentGraph->notifyLinkDisconnected (links[i]);

           break;
        }
    }

DBG ("END >>> GraphConnectorComponent::removeLinkWith (GraphConnectorComponent* connector)");
}

//==============================================================================
void GraphConnectorComponent::removeLinkWith (GraphNodeComponent* node, const bool notifyListeners)
{
DBG ("START >>> GraphConnectorComponent::removeLinkWith (GraphNodeComponent* node)");

    for (int i = 0; i < links.size (); i++)
    {
        if ((links[i]->from && links[i]->from->getParentGraphComponent () == node)
            || (links[i]->to && links[i]->to->getParentGraphComponent () == node))
        {
            if (notifyListeners && parentGraph)
                parentGraph->notifyLinkDisconnected (links[i]);
        
           break;
        }
    }

DBG ("END >>> GraphConnectorComponent::removeLinkWith (GraphNodeComponent* node)");
}

//==============================================================================
void GraphConnectorComponent::destroyAllLinks (const bool notifyListeners)
{
    for (int i = 0; i < links.size (); i++)
    {
        if (notifyListeners && parentGraph)
            parentGraph->notifyLinkDisconnected (links[i]);
    }
}

//==============================================================================
bool GraphConnectorComponent::connectFrom (GraphLinkComponent* link)
{
    if (canConnectFrom (link))
    {
        link->to = this;
        addLink (link);

        return true;
    }

    return false;
}

//==============================================================================
bool GraphConnectorComponent::connectTo (GraphLinkComponent* link)
{
    if (canConnectTo (link))
    {
        link->from = this;
        addLink (link);

        return true;
    }

    return false;
}

//==============================================================================
void GraphConnectorComponent::notifyGraphChanged()
{
    if (parentGraph)
        parentGraph->notifyGraphChanged ();
}

//==============================================================================
GraphConnectorComponentInput::GraphConnectorComponentInput (GraphNodeComponent* parentGraph_,
                                                            const int connectorType_)
  : GraphConnectorComponent (parentGraph_, connectorType_)
{
}

void GraphConnectorComponentInput::updatePosition()
{
    GraphConnectorComponent::updatePosition();

    int centre_x = getBounds ().getCentreX ();
    int centre_y = getBounds ().getCentreY ();

    // update links    start points
    for (int i = 0; i < links.size (); i++)
    {
        links[i]->setEndPoint (centre_x, centre_y);
    }
}

bool GraphConnectorComponentInput::canConnectFrom (GraphLinkComponent* link)
{
    // TODO - what if already connected ?
    bool alreadyConnected = false;
    Array<GraphConnectorComponent*> linked;
    getLinkedConnectors (linked);
    for (int i = 0; i < linked.size (); i++)
    {
        if (link && linked.getUnchecked (i) == link->from)
        {
            alreadyConnected = true;
            break;
        }
    }

    return link->from
           && (link->from->getType () == getType ())
           && (! alreadyConnected)
           && (! parentGraph->isBeforeNode (link->from->getParentGraphComponent ()));
}

void GraphConnectorComponentInput::getLinkedConnectors (Array<GraphConnectorComponent*>& linked)
{
    for (int i = 0; i < links.size (); i++)
    {
        linked.add (links[i]->from);
    }
}

void GraphConnectorComponentInput::mouseDrag (const MouseEvent& e)
{
    if (e.mods.isLeftButtonDown())
    {
        if (! newLink)
        {
            newLink = new GraphLinkComponent (leftToRight);
            newLink->to = this;

            getParentComponent()->addAndMakeVisible (newLink);

            int centre_x = getBounds ().getCentreX ();
            int centre_y = getBounds ().getCentreY ();

            newLink->setInterceptsMouseClicks (false, false);
            newLink->setEndPoint (centre_x, centre_y);
            newLink->toBack ();
        }

        if (newLink)
        {
            newLink->setStartPoint (getX() + e.x, getY() + e.y);

            Component* hitComponent = getParentComponent()->getComponentAt (getX() + e.x, getY() + e.y);
            GraphConnectorComponent* connector = dynamic_cast<GraphConnectorComponentOutput*> (hitComponent);

            if (connector != currentlyDraggingOver)
            {
                if (currentlyDraggingOver != 0)
                    currentlyDraggingOver->connectionDragExit();

                currentlyDraggingOver = connector;

                if (connector
                    && connector->getParentGraphComponent () != parentGraph
                    && connector->canConnectTo (newLink))
                {
                    currentlyDraggingOver->connectionDragEnter();
                }
            }
        }
    }
}

void GraphConnectorComponentInput::mouseUp (const MouseEvent& e)
{
    if (newLink)
    {
        if (currentlyDraggingOver != 0)
        {
            currentlyDraggingOver->connectionDragExit();
            currentlyDraggingOver = 0;
        }

        Component* hitComponent = getParentComponent()->getComponentAt (getX() + e.x, getY() + e.y);
        GraphConnectorComponent* connector = dynamic_cast<GraphConnectorComponentOutput*> (hitComponent);

        if (connector)
        {
            if (connector->getParentGraphComponent () != parentGraph
                && connector->connectTo (newLink))
            {
                newLink->setStartPoint (connector->getX() + connector->getWidth() / 2,
                                        connector->getY() + connector->getHeight() / 2);
                addLink (newLink);

                notifyGraphChanged ();

                newLink->setInterceptsMouseClicks (true, false);
                newLink->repaint();
                newLink = 0;
                return;
            }
        }

        deleteAndZero (newLink);
    }
}


//==============================================================================
GraphConnectorComponentOutput::GraphConnectorComponentOutput (GraphNodeComponent* parentGraph_,
                                                              const int connectorType_)
  : GraphConnectorComponent (parentGraph_, connectorType_)
{
}

void GraphConnectorComponentOutput::updatePosition()
{
    GraphConnectorComponent::updatePosition();

    int centre_x = getBounds ().getCentreX ();
    int centre_y = getBounds ().getCentreY ();

    //update links end points
    for (int i = 0; i < links.size (); i++)
    {
        links[i]->setStartPoint (centre_x, centre_y);
    }
}

bool GraphConnectorComponentOutput::canConnectTo (GraphLinkComponent* link)
{
    bool alreadyConnected = false;
    Array<GraphConnectorComponent*> linked;
    getLinkedConnectors (linked);
    for (int i = 0; i < linked.size (); i++)
    {
        if (link && linked.getUnchecked (i) == link->to)
        {
            alreadyConnected = true;
            break;
        }
    }

    return link->to
           && (link->to->getType () == getType ())
           && (! alreadyConnected)
           && (! parentGraph->isAfterNode (link->to->getParentGraphComponent ()));
}

void GraphConnectorComponentOutput::getLinkedConnectors(Array<GraphConnectorComponent*>& linked)
{
    for (int i = 0; i < links.size (); i++)
    {
        linked.add (links[i]->to);
    }
}

void GraphConnectorComponentOutput::mouseDrag (const MouseEvent& e)
{
    if (e.mods.isLeftButtonDown())
    {
        if (! newLink)
        {
            newLink = new GraphLinkComponent (leftToRight);
            newLink->from = this;
            getParentComponent()->addAndMakeVisible (newLink);

            int centre_x = getBounds ().getCentreX ();
            int centre_y = getBounds ().getCentreY ();

            newLink->setInterceptsMouseClicks (false, false);
            newLink->setStartPoint (centre_x, centre_y);
            newLink->toBack ();
        }

        if (newLink)
        {
            newLink->setEndPoint (getX() + e.x, getY() + e.y);

            Component* hitComponent = getParentComponent()->getComponentAt (getX() + e.x, getY() + e.y);
            GraphConnectorComponent* connector = dynamic_cast<GraphConnectorComponentInput*> (hitComponent);

            if (connector != currentlyDraggingOver)
            {
                if (currentlyDraggingOver != 0)
                    currentlyDraggingOver->connectionDragExit();

                currentlyDraggingOver = connector;

                if (connector
                    && connector->getParentGraphComponent () != parentGraph
                    && connector->canConnectFrom (newLink))
                {
                    currentlyDraggingOver->connectionDragEnter();
                }
            }
        }
    }
}

void GraphConnectorComponentOutput::mouseUp (const MouseEvent& e)
{
    if (newLink)
    {
        if (currentlyDraggingOver != 0)
        {
            currentlyDraggingOver->connectionDragExit();
            currentlyDraggingOver = 0;
        }

        Component* hitComponent = getParentComponent()->getComponentAt (getX() + e.x, getY() + e.y);
        GraphConnectorComponent* connector = dynamic_cast<GraphConnectorComponentInput*> (hitComponent);

        if (connector)
        {
            if (connector->getParentGraphComponent () != parentGraph
                && connector->connectFrom (newLink))
            {
                newLink->setEndPoint (connector->getX() + connector->getWidth() / 2,
                                      connector->getY() + connector->getHeight() / 2);
                addLink (newLink);

                notifyGraphChanged ();

                newLink->setInterceptsMouseClicks (true, false);
                newLink->repaint();
                newLink = 0;
                return;
            }
        }

        deleteAndZero (newLink);
    }
}

END_JUCE_NAMESPACE
