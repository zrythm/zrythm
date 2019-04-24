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

//==============================================================================
ImageSlider::ImageSlider (const String& sliderName)
  : ParameterSlider (sliderName),
    defaultValue (0.5f),
    minOffset (0),
    maxOffset (0),
    thumbImage (0)
{
    setOrientation (ImageSlider::LinearVertical);
}

ImageSlider::~ImageSlider()
{

}

//==============================================================================
void ImageSlider::setOrientation (const SliderOrientation orientation)
{
    if (orientation == ImageSlider::LinearHorizontal)
    {
        setSliderStyle (Slider::LinearHorizontal);
        setThumbImage (ImageCache::getFromMemory (ImageSlider::fader_horizontal,
                                                  ImageSlider::fader_horizontal_size));
        setThumbOffsetInImage (4, 4); // used if the thumb is smaller than
                                        // the size of its image, for example
                                        // if the thumb have a inner shadow
    }
    else if (orientation == ImageSlider::LinearVertical)
    {
        setSliderStyle (Slider::LinearVertical);
        setThumbImage (ImageCache::getFromMemory (ImageSlider::fader_vertical,
                                                  ImageSlider::fader_vertical_size));
        setThumbOffsetInImage (4, 4); // used if the thumb is smaller than
                                        // the size of its image, for example
                                        // if the thumb have a inner shadow
    }
}

//==============================================================================
void ImageSlider::setThumbImage (Image newImage)
{
    thumbImage = newImage;
}

void ImageSlider::setThumbOffsetInImage (const int newMinOffset, const int newMaxOffset)
{
    minOffset = newMinOffset;
    maxOffset = newMaxOffset;
}

void ImageSlider::setDefaultValue (const float newDefault)
{
    defaultValue = newDefault;
}

//==============================================================================
void ImageSlider::paint (Graphics& g)
{
    float x = 0, y = 0;
    int minX, maxX;
    const bool sliderEnabled = isEnabled();
    const bool hightlight = isMouseOverOrDragging();

    if (getSliderStyle() == Slider::LinearVertical)
    {
        x = (getWidth () - thumbImage.getWidth()) / 2;
        y = (getHeight () - thumbImage.getHeight()) * (1.0f - valueToProportionOfLength (getValue()));
        minX = minOffset;
        maxX = getHeight() - maxOffset;

        // draw background line
        g.setColour (Colours::black.withAlpha (sliderEnabled ? 0.5f : 0.3f));
        g.drawVerticalLine (getWidth() / 2,
                            minX, jmax ((int) y + minOffset, minX));
        g.drawVerticalLine (getWidth() / 2,
                            jmin ((int)y + thumbImage.getHeight() - maxOffset, maxX), maxX);
    }
    else if (getSliderStyle() == Slider::LinearHorizontal)
    {
        x = (getWidth () - thumbImage.getWidth()) * valueToProportionOfLength (getValue());
        y = (getHeight () - thumbImage.getHeight()) / 2;
        minX = minOffset;
        maxX = getWidth() - maxOffset;

        // draw background line
        g.setColour (Colours::black.withAlpha (sliderEnabled ? 0.5f : 0.3f));
        g.drawHorizontalLine (getHeight() / 2,
                                minX, jmax ((int) x + minOffset, minX));
        g.drawHorizontalLine (getHeight() / 2,
                                jmin ((int) x + thumbImage.getWidth() - maxOffset, maxX), maxX);
    }

    if (thumbImage.isValid())
    {
        if (! sliderEnabled)
            g.setOpacity (0.6f);
        else
            g.setOpacity (1.0f);

        // draw thumb image
        g.drawImage (thumbImage,
                        (int) x, (int) y, thumbImage.getWidth(), thumbImage.getHeight(),
                        0, 0, thumbImage.getWidth(), thumbImage.getHeight());

        if (sliderEnabled && hightlight)
        {
            // g.setColour (Colours::yellow.withAlpha (0.25f));
            // g.fillRect ((int) x, (int) y, thumbImage.getWidth(), thumbImage.getHeight());
        }
    }
}


//==============================================================================
static const unsigned char temp1[] = {137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,24,0,0,0,15,8,6,0,0,0,254,164,15,219,0,0,0,6,98,75,71,
  68,0,255,0,255,0,255,160,189,167,147,0,0,0,9,112,72,89,115,0,0,11,19,0,0,11,19,1,0,154,156,24,0,0,0,7,116,73,77,69,
  7,215,3,14,16,36,13,247,239,181,191,0,0,1,187,73,68,65,84,56,203,213,148,221,110,211,64,16,133,191,93,219,113,32,70,212,49,229,231,130,
  86,165,42,80,94,142,39,224,9,120,48,238,145,42,168,194,5,106,65,105,73,90,215,78,213,198,206,122,135,11,79,132,228,70,194,149,184,97,165,213,
  104,87,51,115,118,230,156,89,248,223,151,249,135,254,210,55,192,232,14,54,248,244,1,88,219,6,144,110,128,57,157,151,23,167,63,231,233,60,207,1,
  33,8,66,30,12,99,70,15,71,28,127,159,178,251,34,229,89,246,24,231,5,196,179,188,185,225,243,209,132,31,103,191,88,148,87,124,252,240,254,57,
  80,3,75,96,21,118,1,146,100,100,231,23,19,142,190,30,147,101,41,73,146,16,199,49,105,154,50,249,244,133,253,157,109,14,118,158,82,57,8,44,
  12,44,204,46,11,78,166,51,198,217,19,128,61,160,4,114,160,8,59,229,7,214,66,24,89,162,65,132,181,33,54,8,48,166,45,116,24,133,88,107,
  105,0,47,128,7,111,160,105,60,198,128,115,13,192,59,96,170,249,150,225,6,2,218,108,34,109,59,229,222,194,217,83,155,3,177,237,230,247,162,188,
  24,243,135,211,251,105,45,5,30,1,67,192,222,1,16,143,113,206,179,170,87,120,239,240,190,65,124,91,70,237,26,68,4,11,88,211,114,96,13,4,
  65,155,70,109,168,10,180,128,233,182,72,202,235,107,198,227,45,14,223,188,198,88,105,57,0,242,60,231,237,203,140,198,57,190,157,156,227,61,8,66,
  125,123,203,217,249,12,16,138,162,0,112,42,81,15,200,29,128,221,237,173,125,237,227,33,240,10,24,3,131,30,179,32,42,207,43,96,1,84,128,15,
  55,56,214,42,179,181,18,230,90,182,249,11,128,215,151,47,52,182,4,170,176,243,130,70,145,47,245,174,0,98,5,232,179,26,29,176,245,28,84,102,
  195,55,17,0,145,170,96,160,103,211,179,69,58,29,84,186,235,223,56,246,168,17,206,84,69,211,0,0,0,0,73,69,78,68,174,66,96,130,0,0};
const char* ImageSlider::fader_horizontal = (const char*) temp1;
const int ImageSlider::fader_horizontal_size = 558;

static const unsigned char temp2[] = {137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,15,0,0,0,24,8,6,0,0,0,37,5,167,105,0,0,0,6,98,75,71,
  68,0,255,0,255,0,255,160,189,167,147,0,0,0,9,112,72,89,115,0,0,11,19,0,0,11,19,1,0,154,156,24,0,0,1,197,73,68,65,84,
  56,203,197,148,219,110,211,64,16,134,191,217,181,227,64,34,81,39,42,136,11,90,149,138,67,121,255,119,224,30,169,64,21,132,170,2,74,75,220,184,
  117,211,180,118,236,29,46,216,36,198,78,137,196,13,150,70,214,206,206,191,115,252,7,254,215,39,255,120,167,15,25,136,23,219,56,235,18,4,84,128,
  54,193,242,45,201,46,251,253,158,49,6,4,196,169,138,58,36,155,205,216,223,221,57,4,10,32,7,22,65,11,252,35,137,147,203,17,65,104,86,202,
  178,116,12,6,59,0,7,64,6,76,129,172,9,182,73,154,114,252,249,132,176,19,130,42,136,176,40,22,28,189,121,13,112,4,140,189,109,17,108,170,
  197,112,24,99,76,224,83,20,156,43,17,163,0,47,125,254,215,192,180,21,182,181,1,253,126,31,99,237,170,60,206,85,191,207,48,0,18,32,2,108,
  203,243,163,110,68,20,69,136,200,170,214,234,116,217,150,14,16,120,145,22,184,247,184,71,28,199,173,100,210,52,109,245,63,104,246,247,228,116,204,232,
  253,39,186,225,250,170,40,43,222,190,24,182,30,108,121,222,127,30,115,184,183,139,49,235,86,169,42,85,89,214,7,134,141,97,63,27,62,225,213,222,
  83,170,154,206,0,95,206,46,182,123,46,157,146,151,224,180,6,22,112,142,237,96,212,97,13,80,51,182,6,20,221,14,190,159,207,233,24,112,242,167,
  231,226,238,110,59,248,195,241,136,201,244,154,170,90,187,182,214,112,126,49,249,43,88,1,253,126,254,147,179,241,4,145,141,20,214,90,66,106,154,38,
  55,217,21,81,199,98,132,149,132,129,97,126,59,195,211,177,122,136,207,161,159,223,3,224,157,255,199,62,194,18,184,2,190,2,31,129,211,13,172,162,
  240,156,93,82,47,241,91,165,2,110,188,62,3,242,38,184,2,238,129,212,79,82,10,116,253,156,56,191,65,50,175,207,101,195,254,178,158,61,145,23,
  83,219,97,203,7,114,160,248,5,194,64,164,44,166,64,137,52,0,0,0,0,73,69,78,68,174,66,96,130,0,0};
const char* ImageSlider::fader_vertical = (const char*) temp2;
const int ImageSlider::fader_vertical_size = 549;

END_JUCE_NAMESPACE
