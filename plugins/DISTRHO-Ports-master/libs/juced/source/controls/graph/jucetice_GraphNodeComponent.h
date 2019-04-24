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

#ifndef __JUCETICE_GRAPHNODECOMPONENT_HEADER__
#define __JUCETICE_GRAPHNODECOMPONENT_HEADER__

#include "jucetice_GraphLinkComponent.h"
#include "jucetice_GraphConnectorComponent.h"
#include "jucetice_GraphNodeListener.h"


//==============================================================================
/**
    A node in a graph, represented by a graphical component with inputs and
    outputs connectors.
*/
class GraphNodeComponent : public Component,
                           public SettableTooltipClient
{
public:
    //==============================================================================
    /** Constructor */
    GraphNodeComponent();

    /** Destructor */
    ~GraphNodeComponent();

    //==============================================================================
    /** Get the unique id that identify this node */
    int getUniqueID () const                     { return uniqueID; }

    /** Sets the id that identify this node */
    void setUniqueID (const int newID)           { uniqueID = newID; }

    //==============================================================================
    /** Return the text displayed */
    const String& getText () const               { return textToDisplay; }

    /** Set the text displayed in the middle of the object */
    void setText (const String& newText)         { textToDisplay = newText; }

    //==============================================================================
    /** Return the colour of the node */
    const Colour& getNodeColour () const         { return nodeColour; }

    /** Set the colour of the node */
    void setNodeColour (const Colour& newColour) { nodeColour = newColour; }

    //==============================================================================
    /** Return if the object is locked, so no dragging is available */
    bool isLocked () const                       { return hasBeenLocked; }

    /** Set if the object is locked, so no draggaing is available */
    void setLocked (const bool lockedState)      { hasBeenLocked = lockedState; }

    //==============================================================================
    /** Return the user data associated with this node */
    void* getUserData () const                   { return userData; }

    /** Set the user data associated with this node */
    void setUserData (void* newUserData)         { userData = newUserData; }

    //==============================================================================
    /** Get the orientation of the node */
    bool isLeftToRight () const                  { return leftToRight; }

    /** Set the orientation of the node */
    void setLeftToRight (const bool isLToR)      { leftToRight = isLToR; }

    //==============================================================================
    /** Return the number of input connectors for this node.

        Inputs connectors are obviously >= 0.
    */
    int getInputConnectorCount () const          { return graphInputs.size (); }

    /** Get an already added input connector.

        You should use the connector id, which is tipically the index
        of the connector in the inputs array.
    */
    GraphConnectorComponentInput* getInputConnector (const int connectorID);

    /** Add a input connector.

        You can specify the id of the new connector you are adding, or
        leave it to be calculated automagically.
    */
    void addInputConnector (const int connectorType = 0);

    /** Returns the index of the first occurrence of an input.
    
        Specify a link type to be checked.
     */
    int getFirstInputOfType (const int connectorType);

    /** TODO - document */
    int getInputLinksCount () const;

    //==============================================================================
    /** Return the number of output connectors for this node.

        Outputs connectors are obviously >= 0
    */
    int getOutputConnectorCount () const         { return graphOutputs.size (); }

    /** Get an already added output connector.

        You should use the connector id, which is tipically the index
        of the connector in the outputs array.
    */
    GraphConnectorComponentOutput* getOutputConnector (const int connectorID);

    /** Add a output connector.

        You can specify the id of the new connector you are adding, or
        leave it to be calculated automagically.
    */
    void addOutputConnector (const int connectorType = 0);

    /** Returns the index of the first occurrence of an output.
    
        Specify a link type to be checked.
     */
    int getFirstOutputOfType (const int connectorType);

    /** TODO - document */
    int getOutputLinksCount () const;

    //==============================================================================
    /** Free all connectors */
    virtual void deleteConnectors (const bool freeConnectors = true);

    //==============================================================================
    /** Connect an output port of this to an input port of another node

        It will return true if the connection has take place, false
        otherwise.

        Eventually you can specify to not notify listeners of the changes,
        typically if you are restoring node to node connections massivley
        and wants to.
    */
    bool connectToNode (const int sourcePort,
                        GraphNodeComponent* destinationNode,
                        const int destinationPort,
                        const bool notifyListener = true);

    //==============================================================================
    /** Break all links of this node.

        This should break all inputs and outputs links between this and other nodes.
        You can optionally set if it have to notify the listener about this changes.
    */
    void breakAllLinks (const bool notifyListener = true);

    /** Break inputs links of this node.

        This should break all inputs links between this and other nodes.
        You can optionally set if it have to notify the listener about this changes.
    */
    void breakInputLinks (const bool notifyListener = true);

    /** Break output links of this node.

        This should break all outputs links between this and other nodes.
        You can optionally set if it have to notify the listener about this changes.
    */
    void breakOutputLinks (const bool notifyListener = true);

    //==============================================================================
    /** Set the listener that manage this Graph */
    void setNodeListener (GraphNodeListener* const listener);

    //==============================================================================
    /** Dispatching for the listener about popup menu clicked on a connector.

        This is typically called from within a connector itself, then dispatched
        to the attached listener.
    */
    void notifyConnectorPopupMenuSelected (GraphConnectorComponent* changedConnector);

    /** Dispatching for the listener about popup menu clicked on a connector.

        This is typically called from within a connector itself, then dispatched
        to the attached listener.
    */
    void notifyLinkPopupMenuSelected (GraphLinkComponent* changedLink);

    /** This is called whenever a node is right clicked */
    void notifyLinkConnected (GraphLinkComponent* changedLink);

    /** This is called whenever a node is right clicked */
    void notifyLinkDisconnected (GraphLinkComponent* changedLink);

    /** Dispatch a call to the listener, about a graph change */
    void notifyGraphChanged ();

    /** Called from a connector when we want to get its colour */
    Colour getConnectorColour (GraphConnectorComponent* connector,
                               const bool isSelected);

    //==============================================================================
    /** Check if this node is already connected AFTER another one */
    bool isAfterNode (GraphNodeComponent* nodeBefore);

    /** Check if this node is already connected BEFORE another one */
    bool isBeforeNode (GraphNodeComponent* nodeAfter);

    //==============================================================================
    /** @internal */
    void mouseDown (const MouseEvent& e);
    /** @internal */
    void mouseDrag (const MouseEvent& e);
    /** @internal */
    void mouseDoubleClick (const MouseEvent& e);
    /** @internal */
    void paint (Graphics& g);
    /** @internal */
    void resized();
    /** @internal */
    void moved();

    //==============================================================================
    juce_UseDebuggingNewOperator

protected:

    //==============================================================================
    /** Called when we move around cause we have to update connectors */
    void updateConnectors();

    Array<GraphConnectorComponentInput*> graphInputs;
    Array<GraphConnectorComponentOutput*> graphOutputs;

    GraphNodeListener* listener;

    void* userData;
    int uniqueID;
    String textToDisplay;
    Colour nodeColour;

    bool leftToRight : 1,
         hasBeenLocked : 1;

    ComponentBoundsConstrainer constrainer;
    int originalX, originalY;
    int lastX, lastY;

    //DropShadower dropShadower;
};

#endif
