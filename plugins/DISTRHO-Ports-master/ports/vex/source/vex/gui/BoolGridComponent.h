/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2008 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2008 by Julian Storer.

 ------------------------------------------------------------------------------

 JUCE and JUCETICE can be redistributed and/or modified under the terms of
 the GNU Lesser General Public License, as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 JUCE and JUCETICE are distributed in the hope that they will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with JUCE and JUCETICE; if not, visit www.gnu.org/licenses or write to
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA

 ==============================================================================

   @author  rockhardbuns
   @tweaker Lucio Asnaghi
   @tweaker falkTX

 ==============================================================================
*/

#ifndef DISTRHO_VEX_BOOLGRIDCOMPONENT_HEADER_INCLUDED
#define DISTRHO_VEX_BOOLGRIDCOMPONENT_HEADER_INCLUDED

#ifdef CARLA_EXPORT
 #include "juce_gui_basics.h"
#else
 #include "../../StandardHeader.h"
#endif

class BoolGridComponent : public Component,
                          public ChangeBroadcaster
{
public:
    BoolGridComponent()
        : Component("Bool Grid Component")
    {
        grid = new bool[80];
        lastCell = -1;
        cellX = -1; cellY = -1;
        sizeX = 16;  sizeY = 5;
        activeLength = 8;

        for(int x = 0; x < sizeX; x++)
        {
            for(int y = 0; y < sizeY; y++)
            {
                grid[(x * sizeY) + y] = false;
            }
        }
    }

    ~BoolGridComponent() override
    {
        delete[] grid;
    }

    void paint(Graphics& g) override
    {
        //background
        //g.setColour(Colour(200,200,210).withAlpha(0.4f));
        //g.fillRect(0,0,getWidth(),getHeight());

        //cell size -recalculate in case of resizing
        cellX = getWidth() / sizeX;
        cellY = getHeight() / sizeY;

        //Draw lines and checkmarks
        for(int x = 0; x < sizeX; x++){
            for(int y = 0; y < sizeY; y++){
                //lines
                g.setColour(Colour(100,100,130));
                g.drawLine((float) x*cellX, 0.0f,(float)  x*cellX, (float) getHeight() );
                g.drawLine(0.0f, (float) y*cellY, (float)getWidth(), (float) y*cellY);

                //checkmarks
                g.setColour(Colours::black);
                if (grid[(x * sizeY) + sizeY - y -1]){
                    g.fillEllipse((float) x*cellX+3, (float) y*cellY+3 ,(float) cellX - 5 ,(float) cellY - 5);
                }
            }
        }

        //Grey stuff out
        g.setColour(Colour(uint8(170),170,170,.7f));
        g.fillRect(cellX*activeLength,0,getWidth(),getHeight());

        //bevel outline for the entire draw area
        LookAndFeel_V2::drawBevel(g, 0, 0, getWidth(), getHeight(), 1, Colours::black, Colours::white, 0);
    }

    //Yay, someone clicked my component!
    void mouseDown(const MouseEvent& e) override
    {
        if (e.mouseWasClicked())
        {
            if ((e.y < getHeight()-1) && (e.x < getWidth()-1)){ //this avoids false triggers along the rims

                int cx = (e.x-1)/cellX; int cy = (e.y-1)/cellY; //cx,cy are the cell coords

                if (cx < activeLength){  //if the click was on the greyed out portion, we dont do jack
                    lastCell = (cx * sizeY) + sizeY - cy -1;
                    grid[lastCell]= !grid[lastCell];  //toggle the clicked cell
                    repaint();
                    sendChangeMessage();
                }
            }
        }
    }

    //get the state of a specific cell
    bool getCellState(int x, int y) const
    {
        return grid[(x * sizeY) + y];
    }

    bool getCellState(int x) const
    {
        return grid[x];
    }

    //set the state of a specific cell. the last param determines if we broadcast
    //Cell determined by grid coords i.e X * Y
    void setCellState(int x, int y, bool state, bool broadcast = false)
    {
        grid[(x * sizeY) + y] = state;
        if (broadcast){sendChangeMessage();}
        repaint();
    }

    //Overloaded - cell determined soley by array index
    void setCellState(int x, bool state, bool broadcast = false)
    {
        grid[x] = state;
        if (broadcast){sendChangeMessage();}
        repaint();
    }

    // the last changed cell, in terms of array index
    int getLastChanged()
    {
        return lastCell;
        lastCell = -1;
    }

    //Get the active length
    int getLength() const
    {
        return activeLength;
    }

    //Set the active length of the grid
    void setLength(int l)
    {
        activeLength = jmin(l, sizeX);
        activeLength = jmax(activeLength, 1);
        repaint();
    }

    //Clear the grid
    void reset()
    {
        for(int x = 0; x < sizeX; x++){
            for(int y = 0; y < sizeY; y++){
                grid[(x * sizeY) + y] = false;
            }
        }
        repaint();
    }

private:
    int sizeX, sizeY; // grid size in number of cells
    int cellX, cellY; // cell size in pixels, used by paint();
    int lastCell;     // last cell clicked, for outside interaction
    int activeLength; // How much of the grid should be usable vs greyed

    bool* grid;  //  SizeX * SizeY

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BoolGridComponent)
};

#endif // DISTRHO_VEX_BOOLGRIDCOMPONENT_HEADER_INCLUDED
