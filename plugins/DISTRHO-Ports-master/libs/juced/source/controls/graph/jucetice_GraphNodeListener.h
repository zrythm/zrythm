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

#ifndef __JUCETICE_GRAPHNODELISTENER_HEADER__
#define __JUCETICE_GRAPHNODELISTENER_HEADER__

#include "jucetice_GraphLinkComponent.h"
#include "jucetice_GraphConnectorComponent.h"

class GraphNodeComponent;

//==============================================================================
/**
    Listener for all stuff that is happening inside a graph node
*/
class GraphNodeListener
{
public:

    //==============================================================================
    /** Public destructor */
    virtual ~GraphNodeListener () {}

    //==============================================================================
    /** This is called when a node needs to be paint */
    virtual void nodePaint (GraphNodeComponent* node, Graphics& g)
    {
    }

    /** This is called when a node is moved */
    virtual void nodeMoved (GraphNodeComponent* node, const int deltaX, const int deltaY)
    {
    }

    /** This is called when a node is selected */
    virtual void nodeSelected (GraphNodeComponent* node)
    {
    }

    /** This is called whenever a node is double clicked */
    virtual void nodeDoubleClicked (GraphNodeComponent* node, const MouseEvent& e)
    {
    }

    /** This is called whenever a node is right clicked */
    virtual void nodePopupMenuSelected (GraphNodeComponent* node)
    {
    }

    //==============================================================================
    /** This is called whenever a node is right clicked */
    virtual void linkConnected (GraphLinkComponent* newLink)
    {
    }

    /** This is called whenever a node is right clicked */
    virtual void linkDisconnected (GraphLinkComponent* oldLink)
    {
    }

    /** This is called whenever a node is right clicked */
    virtual void linkPopupMenuSelected (GraphLinkComponent* link)
    {
    }

    //==============================================================================
    /** This is called when a connector need to be painted */
    virtual Colour getConnectorColour (GraphConnectorComponent* connector,
                                       const bool isSelected)
    {
        return isSelected ? Colours::red : Colours::black;
    }

    /** This is called when a connector is right clicked */
    virtual void connectorPopupMenuSelected (GraphConnectorComponent* connector)
    {
    }

    //==============================================================================
    /** This is called whenever new graph change */
    virtual void graphChanged ()
    {
    }

protected:

    /** Protected constructor */
    GraphNodeListener () {}
};

#endif
