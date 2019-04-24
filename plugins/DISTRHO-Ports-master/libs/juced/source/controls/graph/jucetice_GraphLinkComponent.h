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

#ifndef __JUCETICE_GRAPHLINKCOMPONENT_HEADER__
#define __JUCETICE_GRAPHLINKCOMPONENT_HEADER__

class GraphConnectorComponent;


//==============================================================================
class GraphLinkComponent : public Component
{
public:

    //==============================================================================
    GraphConnectorComponent* from;
    GraphConnectorComponent* to;

    //==============================================================================
    GraphLinkComponent (const bool leftToRight,
                        const int lineThickness = 2,
                        const float tension = 0.45f);

    ~GraphLinkComponent ();

    //==============================================================================
    void setStartPoint (const int x, const int y);
    void setEndPoint (const int x, const int y);

    //==============================================================================
    void setTension (const float tension);

    //==============================================================================
    /** @internal */
    void mouseUp (const MouseEvent& e);
    /** @internal */
    void mouseDrag (const MouseEvent& e);
    /** @internal */
    void paint (Graphics& graphics);
    /** @internal */
    bool hitTest (int x, int y);

    //==============================================================================
    juce_UseDebuggingNewOperator

protected:

    //==============================================================================
    void updateBounds();

    Path path;

    int startPointX;
    int startPointY;
    int endPointX;
    int endPointY;

    bool leftToRight;
    int lineThickness;
    float tension;
};

#endif
