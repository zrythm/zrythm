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


//==============================================================================
GraphNodeComponent::GraphNodeComponent ()
  : listener (0),
    userData (0),
    uniqueID (0),
    nodeColour (Colours::grey),
    leftToRight (true),
    hasBeenLocked (false),
    originalX (0),
    originalY (0),
    lastX (0),
    lastY (0)/*,
    dropShadower (0.5f, 0, 1, 2.0f)*/
{
    setBufferedToImage (false);
    setOpaque (true);
    toFront (true);

    constrainer.setMinimumOnscreenAmounts (9999,9999,9999,9999);

    //dropShadower.setOwner (this);
}

GraphNodeComponent::~GraphNodeComponent()
{
}

//==============================================================================
void GraphNodeComponent::moved()
{
    updateConnectors();

    // update last position            
    lastX = getX ();
    lastY = getY ();
}

void GraphNodeComponent::resized()
{
    updateConnectors();
}

void GraphNodeComponent::mouseDoubleClick (const MouseEvent& e)
{
    // notify listener
    if (listener)
        listener->nodeDoubleClicked (this, e);
}

void GraphNodeComponent::mouseDown (const MouseEvent& e)
{
    if (listener)
        listener->nodeSelected (this);

    if (e.mods.isRightButtonDown())
    {
        if (listener)
            listener->nodePopupMenuSelected (this);
    }
    else if (e.mods.isLeftButtonDown())
    {    
        if (! hasBeenLocked)
        {
            originalX = getX();
			originalY = getY();
        }
    }
}

void GraphNodeComponent::mouseDrag (const MouseEvent& e)
{
    if (! hasBeenLocked && e.mods.isLeftButtonDown())
    {
        int x = originalX + e.getDistanceFromDragStartX();
        int y = originalY + e.getDistanceFromDragStartY();
        int w = getWidth();
        int h = getHeight();
                
        const Component* const parentComp = getParentComponent();
        Rectangle<int> limits;
        limits.setSize (parentComp->getWidth(), parentComp->getHeight());

		Rectangle<int> bounds (x, y, w, h);
        constrainer.checkBounds (bounds,
                                 getBounds(), limits,
                                 false, false, false, false);

        if (listener)
            listener->nodeMoved (this, x - lastX, y - lastY);

        constrainer.applyBoundsToComponent (*this, bounds);
    }
}

//==============================================================================
void GraphNodeComponent::paint (Graphics& g)
{
    if (listener)
        listener->nodePaint (this, g);
}

//==============================================================================
void GraphNodeComponent::addInputConnector (const int connectorType)
{
    GraphConnectorComponentInput* connector =
        new GraphConnectorComponentInput (this, connectorType);

    connector->setConnectorID (graphInputs.size());

    getParentComponent()->addAndMakeVisible (connector);

    graphInputs.add (connector);

    updateConnectors();
}

void GraphNodeComponent::addOutputConnector (const int connectorType)
{
    GraphConnectorComponentOutput* connector =
        new GraphConnectorComponentOutput (this, connectorType);

    connector->setConnectorID (graphOutputs.size());

    getParentComponent()->addAndMakeVisible (connector);

    graphOutputs.add (connector);

    updateConnectors();
}

//==============================================================================
GraphConnectorComponentInput* GraphNodeComponent::getInputConnector (const int connectorID)
{
    for (int i = 0; i < graphInputs.size(); i++)
    {
        GraphConnectorComponentInput* input = graphInputs.getUnchecked (i);

        if (input && (input->getConnectorID() == connectorID))
        {
            return input;
        }
    }

    return 0;
}

GraphConnectorComponentOutput* GraphNodeComponent::getOutputConnector (const int connectorID)
{
    for (int i = 0; i < graphOutputs.size(); i++)
    {
        GraphConnectorComponentOutput* output = graphOutputs.getUnchecked (i);

        if (output && (output->getConnectorID() == connectorID))
        {
            return output;
        }
    }

    return 0;
}

//==============================================================================
int GraphNodeComponent::getFirstInputOfType (const int connectorType)
{
    for (int i = 0; i < graphInputs.size(); i++)
    {
        GraphConnectorComponentInput* input = graphInputs.getUnchecked (i);

        if (input && (input->getType() == connectorType))
        {
            return i;
        }
    }

    return -1;
}

int GraphNodeComponent::getFirstOutputOfType (const int connectorType)
{
    for (int i = 0; i < graphOutputs.size(); i++)
    {
        GraphConnectorComponentOutput* output = graphOutputs.getUnchecked (i);

        if (output && (output->getType() == connectorType))
        {
            return i;
        }
    }

    return -1;
}

//==============================================================================
int GraphNodeComponent::getInputLinksCount () const
{
    int linksCount = 0;

    Array<GraphConnectorComponent*> links;
    for (int i = 0; i < graphInputs.size(); i++)
    {
        graphInputs.getUnchecked (i)->getLinkedConnectors (links);
        linksCount += links.size ();
    }

    return linksCount;
}

int GraphNodeComponent::getOutputLinksCount () const
{
    int linksCount = 0;

    Array<GraphConnectorComponent*> links;
    for (int i = 0; i < graphOutputs.size(); i++)
    {
        graphOutputs.getUnchecked (i)->getLinkedConnectors (links);
        linksCount += links.size ();
    }

    return linksCount;
}

//==============================================================================
void GraphNodeComponent::deleteConnectors (const bool freeConnectors)
{
    breakAllLinks (false);

    if (freeConnectors) {
        for (int i = 0; i < graphInputs.size(); i++)
            delete graphInputs.getUnchecked (i);
    }

    if (freeConnectors) {
        for (int i = 0; i < graphOutputs.size(); i++)
            delete graphOutputs.getUnchecked (i);
    }

    graphInputs.clear ();
    graphOutputs.clear ();
}

//==============================================================================
void GraphNodeComponent::breakAllLinks (const bool notifyListeners)
{
    for (int i = 0; i < graphInputs.size(); i++)
        graphInputs.getUnchecked (i)->destroyAllLinks (notifyListeners);

    for (int i = 0; i < graphOutputs.size(); i++)
        graphOutputs.getUnchecked (i)->destroyAllLinks (notifyListeners);

    if (notifyListeners)
        notifyGraphChanged ();
}

void GraphNodeComponent::breakInputLinks (const bool notifyListeners)
{
    for (int i = 0; i < graphInputs.size(); i++)
        graphInputs.getUnchecked (i)->destroyAllLinks (notifyListeners);

    if (notifyListeners)
        notifyGraphChanged ();
}

void GraphNodeComponent::breakOutputLinks (const bool notifyListeners)
{
    for (int i = 0; i < graphOutputs.size(); i++)
        graphOutputs.getUnchecked (i)->destroyAllLinks (notifyListeners);

    if (notifyListeners)
        notifyGraphChanged ();
}

//==============================================================================
void GraphNodeComponent::setNodeListener (GraphNodeListener* const listener_)
{
    listener = listener_;
}

void GraphNodeComponent::notifyConnectorPopupMenuSelected (GraphConnectorComponent* changedConnector)
{
    if (listener)
        listener->connectorPopupMenuSelected (changedConnector);
}

void GraphNodeComponent::notifyLinkPopupMenuSelected (GraphLinkComponent* changedLink)
{
    if (listener)
        listener->linkPopupMenuSelected (changedLink);
}

void GraphNodeComponent::notifyLinkConnected (GraphLinkComponent* changedLink)
{
    if (listener)
        listener->linkConnected (changedLink);
}

void GraphNodeComponent::notifyLinkDisconnected (GraphLinkComponent* changedLink)
{
    if (listener)
        listener->linkDisconnected (changedLink);
}

void GraphNodeComponent::notifyGraphChanged ()
{
    if (listener)
        listener->graphChanged ();
}

Colour GraphNodeComponent::getConnectorColour (GraphConnectorComponent* connector,
                                               const bool isSelected)
{
    if (listener)
        return listener->getConnectorColour (connector, isSelected);

    return Colours::black;
}

//==============================================================================
bool GraphNodeComponent::connectToNode (const int sourcePort,
                                        GraphNodeComponent* destinationNode,
                                        const int destinationPort,
                                        const bool notifyListeners)
{
    GraphConnectorComponentOutput* out = getOutputConnector (sourcePort);
    GraphConnectorComponentInput* in = destinationNode->getInputConnector (destinationPort);

    if (out != 0 && in != 0)
    {
        GraphLinkComponent* newLink = new GraphLinkComponent (leftToRight);
        newLink->from = out;

        if (in->canConnectFrom (newLink))
        {
            int startX = out->getBounds ().getCentreX ();
            int startY = out->getBounds ().getCentreY ();
            int endX = in->getBounds ().getCentreX ();
            int endY = in->getBounds ().getCentreY ();

            newLink->setStartPoint (startX, startY);
            newLink->setEndPoint (endX, endY);

            if (in->connectFrom (newLink))
            {
                out->addLink (newLink);

                if (notifyListeners)
                    notifyGraphChanged ();

                getParentComponent()->addAndMakeVisible (newLink);
                newLink->toBack ();
                newLink->repaint();
                return true;
            }
        }

        deleteAndZero (newLink);
    }

    return false;
}

//==============================================================================
bool GraphNodeComponent::isAfterNode (GraphNodeComponent* nodeBefore)
{
    bool findConnector = false;

    for (int j = getInputConnectorCount (); --j >= 0;)
    {
        GraphConnectorComponent* connector = getInputConnector (j);
        if (connector)
        {
            Array <GraphConnectorComponent*> linked;
            connector->getLinkedConnectors (linked);

            for (int c = linked.size (); --c >= 0;)
            {
                GraphConnectorComponent* other = linked.getUnchecked (c);
                if (other)
                {
                    if (other->getParentGraphComponent() == nodeBefore)
                    {
                        return true;
                    }
                    else
                    {
                        findConnector = other->getParentGraphComponent()->isAfterNode (nodeBefore);
                    }
                }

                if (findConnector)
                    return true;
            }
        }
    }

    return findConnector;
}

bool GraphNodeComponent::isBeforeNode (GraphNodeComponent* nodeAfter)
{
    bool findConnector = false;

    for (int j = getOutputConnectorCount (); --j >= 0;)
    {
        GraphConnectorComponent* connector = getOutputConnector (j);
        if (connector)
        {
            Array <GraphConnectorComponent*> linked;
            connector->getLinkedConnectors (linked);

            for (int c = linked.size (); --c >= 0;)
            {
                GraphConnectorComponent* other = linked.getUnchecked (c);
                if (other)
                {
                    if (other->getParentGraphComponent() == nodeAfter)
                    {
                        return true;
                    }
                    else
                    {
                        findConnector = other->getParentGraphComponent()->isBeforeNode (nodeAfter);
                    }
                }

                if (findConnector)
                    return true;
            }
        }
    }

    return findConnector;
}

//==============================================================================
void GraphNodeComponent::updateConnectors()
{
    float x_pos, y_pos, y_pos_step;
    float height, width;

    if (leftToRight)
    {
        height = (float) getHeight();
        width = (float) getWidth();
    }
    else
    {
        width = (float) getHeight();
        height = (float) getWidth();
    }

    if (graphInputs.size() > 0)
    {
        y_pos_step = height / (float) graphInputs.size();

        x_pos = 0.0;
        y_pos = (height / 2) - (((float)y_pos_step * ((float) graphInputs.size() - 1.0f)) / 2.0f);
        for (int i = 0; i < graphInputs.size(); i++)
        {
            if (leftToRight)
                graphInputs[i]->setRelativePosition ((int) (x_pos - graphInputs[i]->getWidth()),
                                                     (int) (y_pos - graphInputs[i]->getWidth() / 2));
            else
                graphInputs[i]->setRelativePosition ((int) (y_pos - graphInputs[i]->getWidth() / 2),
                                                     (int) (x_pos - graphInputs[i]->getWidth()));
            y_pos += y_pos_step;
        }
    }

    if (graphOutputs.size() > 0)
    {
        y_pos_step = height / (float) graphOutputs.size();

        x_pos = width;
        y_pos = (height / 2) - (((float)y_pos_step * ((float) graphOutputs.size() - 1.0f)) / 2.0f);
        for(int i = 0; i < graphOutputs.size(); i++)
        {
            if (leftToRight)
                graphOutputs[i]->setRelativePosition ((int) (x_pos),
                                                      (int) (y_pos - graphOutputs[i]->getWidth() / 2));
            else
                graphOutputs[i]->setRelativePosition ((int) (y_pos - graphOutputs[i]->getWidth() / 2),
                                                      (int) (x_pos));
            y_pos += y_pos_step;
        }
    }
}

END_JUCE_NAMESPACE
