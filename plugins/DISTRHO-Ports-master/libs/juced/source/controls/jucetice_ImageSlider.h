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

#ifndef __JUCETICE_IMAGESLIDER_HEADER__
#define __JUCETICE_IMAGESLIDER_HEADER__

#include "jucetice_ParameterSlider.h"


//==============================================================================
/**
     An image based slider.
*/
class ImageSlider : public ParameterSlider
{
public:

    //==============================================================================
    enum SliderOrientation
    {
        LinearHorizontal = 0,
        LinearVertical   = 1
    };

    //==============================================================================
    /** Constructor */
    ImageSlider (const String& sliderName);

    /** Destructor */
    ~ImageSlider();

    //==============================================================================
    void setOrientation (const SliderOrientation isVertical);

    //==============================================================================
    void setThumbImage (Image newImage);
    void setThumbOffsetInImage (const int newMinOffset, const int newMaxOffset);

    //==============================================================================
    /** Set the default value */
    void setDefaultValue (const float newDefault);

    //==============================================================================
    /** @internal */
    void paint (Graphics& g);

    //==============================================================================
    juce_UseDebuggingNewOperator

private:

    //==============================================================================
    /** Image handle resources */
    static const char*  fader_horizontal;
    static const int    fader_horizontal_size;
    static const char*  fader_vertical;
    static const int    fader_vertical_size;


    float defaultValue;
    int minOffset, maxOffset;
    Image thumbImage;
};


#endif // __JUCETICE_IMAGESLIDER_HEADER__
