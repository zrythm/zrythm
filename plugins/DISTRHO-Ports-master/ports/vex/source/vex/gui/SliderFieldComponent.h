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

#ifndef DISTRHO_VEX_SLIDERFIELDCOMPONENT_HEADER_INCLUDED
#define DISTRHO_VEX_SLIDERFIELDCOMPONENT_HEADER_INCLUDED

#ifdef CARLA_EXPORT
 #include "juce_gui_basics.h"
#else
 #include "../../StandardHeader.h"
#endif

class SliderFieldComponent : public Component,
                             public ChangeBroadcaster
{
public:
    SliderFieldComponent()
      : Component("Slider Field Component")
    {
        array = new float[16];
        lastSlider = -1;   // -1, not to test against really
        sliderWidth = -1;  // but it makes fuckups more visible
        numSliders = 16;
        activeLength = 8;

        //empty out the array.. to make sure
        for(int i = 0; i < numSliders; i++)
              array[i] = 0.5f;
    }

    ~SliderFieldComponent() override
    {
        delete[] array;
        deleteAllChildren();
    }

     void paint(Graphics& g) override
     {
        //cell size -recalculate in case of resizing
        sliderWidth = getWidth() / numSliders;

        //int middle = int(getHeight() * 0.5f);

        //Draw bars
        g.setColour(Colour(50,50,50));
        for(int i = 0; i < numSliders; i++)
        {
              if(array[i] > 0.0f){
                  g.fillRect(i * sliderWidth + 2, getHeight() - int(array[i] * getHeight()), sliderWidth - 4, int(getHeight() * array[i])) ;
              }
        }

        g.setColour(Colour(100,100,130));

        //Grey stuff out
        g.setColour(Colour(uint8(170),170,170,.7f));
        g.fillRect(sliderWidth*activeLength,0,getWidth(),getHeight());

        //bevel outline for the entire draw area
        LookAndFeel_V2::drawBevel(g, 0, 0, getWidth(), getHeight(), 1, Colours::black, Colours::white, 0);
     }

     void mouseDrag(const MouseEvent& e) override
     {
        if ((e.y < getHeight()-1) && (e.x < getWidth()-1))
        {
            //this avoids false triggers along the rims
            float height = (float) getHeight();
            int index = (e.x-1)/sliderWidth;
            float value = ((height - e.y -1.0f) / height);

            //if the click was on the greyed out portion, we dont do jack
            if (index < activeLength)
            {
                lastSlider = index;
                array[index]= value;
                repaint();
                sendChangeMessage();
            }
        }
     }

     void mouseDown(const MouseEvent& e) override
     {
        if ((e.y < getHeight()-1) && (e.x < getWidth()-1))
        {
            //this avoids false triggers along the rims
            float height = (float) getHeight();
            int index = (e.x-1)/sliderWidth;
            float value = ((height - e.y ) / height);

            //if the click was on the greyed out portion, we dont do jack
            if (index < activeLength)
            {
                lastSlider = index;
                array[index]= value;
                repaint();
                sendChangeMessage();
            }
        }
     }

     int getLastSlider() const
     {
          return lastSlider;
     }

     float getValue(int i) const
     {
          return array[i];
     }

     void setValue(int i, float v) const
     {
          array[i] = v;
     }

    int getLength() const
    {
        return activeLength;
    }

    void setLength(const int l)
    {
        activeLength = jmin(l, numSliders);
        activeLength = jmax(activeLength, 1);
        repaint();
    }

    void reset()
    {
        //empty out the array.. to make sure
        for(int i = 0; i < numSliders; i++)
            array[i] = 0.0f;

        repaint();
    }

private:
     int numSliders;
     int sliderWidth;
     int lastSlider;   // last cell clicked, for outside interaction
     int activeLength; // How much of the grid should be usable vs greyed

    float* array;  // SizeX

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SliderFieldComponent)
};

#endif // DISTRHO_VEX_SLIDERFIELDCOMPONENT_HEADER_INCLUDED
