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

#ifndef __JUCETICE_GRAPHCONNECTORCOMPONENT_HEADER__
#define __JUCETICE_GRAPHCONNECTORCOMPONENT_HEADER__

class GraphNodeComponent;
class GraphLinkComponent;


//==============================================================================
class GraphConnectorComponent : public Component
{
public:

    //==============================================================================
    /** Connector constructor */
    GraphConnectorComponent (GraphNodeComponent* parentGraph,
                             const int connectorType = 0);

    /** Destructor */
    ~GraphConnectorComponent ();

    //==============================================================================
    /** Return the unique identifier for this connector

        This unique identify the connector in its node, so different node
        connectors can have the same id.
    */
    int getConnectorID () const           { return connectorID; }

    /** Set the unique identifier for this connector */
    void setConnectorID (const int newID) { connectorID = newID; }

    //==============================================================================
    /** Returns the connector type

        Different connector types represent different kind of connections available
        so a connector of a type can't be connected to one of another type.
    */
    int getType () const                  { return connectorType; }

    /** Change the connector type. */
    void setType (const int newType)      { connectorType = newType; }

    //==============================================================================
    /** Returns the connector type */
    virtual bool isInput () const = 0;

    //==============================================================================
    virtual void getLinkedConnectors (Array<GraphConnectorComponent*>& links) = 0;
    virtual bool canConnectFrom (GraphLinkComponent* link) { return false; }
    virtual bool canConnectTo (GraphLinkComponent* link) { return false; }
    virtual void updatePosition();

    //==============================================================================
    void setRelativePosition (const int x, const int y);

    //==============================================================================
    bool connectTo (GraphLinkComponent* link);
    bool connectFrom (GraphLinkComponent* link);
    void notifyGraphChanged ();

    //==============================================================================
    void addLink (GraphLinkComponent* newLink);
    void removeLink (GraphLinkComponent* newLink);
    void removeLinkWith (GraphConnectorComponent* connector, const bool notifyListeners = true);
    void removeLinkWith (GraphNodeComponent* node, const bool notifyListeners = true);
    void destroyAllLinks (const bool notifyListeners = true);

    //==============================================================================
    GraphNodeComponent* getParentGraphComponent () const { return parentGraph; }

    //==============================================================================
    /** @internal */
    void connectionDragEnter();
    /** @internal */
    void connectionDragExit();
    /** @internal */
    void mouseDown (const MouseEvent& e);
    /** @internal */
    void paint (Graphics& g);

    //==============================================================================
    juce_UseDebuggingNewOperator

protected:

    GraphNodeComponent* parentGraph;
    int relativePositionX, relativePositionY;

    OwnedArray<GraphLinkComponent> links;
    GraphLinkComponent* newLink;
    GraphConnectorComponent* currentlyDraggingOver;

//    Colour normalColour, selectedColour;

    int connectorID, connectorType;
    bool connectionDraggedOver;
    bool leftToRight;
};

//==============================================================================
class GraphConnectorComponentInput : public GraphConnectorComponent
{
public:
    GraphConnectorComponentInput (GraphNodeComponent* parentGraph_,
                                  const int connectorType_);

    bool isInput () const { return true; }

    bool canConnectFrom (GraphLinkComponent* link);
    void getLinkedConnectors (Array<GraphConnectorComponent*>& links);
    void updatePosition();

    void mouseDrag(const MouseEvent& e);
    void mouseUp(const MouseEvent& e);
};

//==============================================================================
class GraphConnectorComponentOutput : public GraphConnectorComponent
{
public:
    GraphConnectorComponentOutput (GraphNodeComponent* parentGraph_,
                                   const int connectorType_);

    bool isInput () const { return false; }

    virtual bool canConnectTo (GraphLinkComponent* link);
    virtual void getLinkedConnectors (Array<GraphConnectorComponent*>& links);
    virtual void updatePosition();

    void mouseDrag(const MouseEvent& e);
    void mouseUp(const MouseEvent& e);
};

#endif
