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
ImageKnob::ImageKnob (const String& sliderName)
  : ParameterSlider (sliderName),
    defaultValue (0.5f),
    stitchedImage (0),
    stitchedOrientation (ImageKnob::Vertical),
    numberOfSubImages (0)
{
    setSliderStyle (Slider::Rotary);
}

ImageKnob::~ImageKnob()
{

}

//==============================================================================
void ImageKnob::setStitchedImage (Image newImage,
                                  StitchOrientation newOrientation,
                                  const int newNumberOfSubImages)
{
    jassert (!newImage.isNull());
    jassert (newNumberOfSubImages > 0);

    stitchedImage = newImage;
	stitchedImage.duplicateIfShared();
    stitchedOrientation = newOrientation;
    numberOfSubImages = newNumberOfSubImages;
}
                           
void ImageKnob::setDefaultValue (const float newDefault)
{
    defaultValue = newDefault;
}

//==============================================================================
void ImageKnob::paint (Graphics& g)
{
    jassert (stitchedImage.isValid()); // you must first assign a stitchedImage to be
                                  // drawn before showing your knob !

    const bool sliderEnabled = isEnabled();
    // const bool hightlight = isMouseOverOrDragging();

    int imageWidth, imageHeight;
    int sourceX, sourceY, offsetX, offsetY;
    int indexImage = roundFloatToInt ((numberOfSubImages - 1) * valueToProportionOfLength (getValue()));

    if (stitchedOrientation == Horizontal)
    {
        imageWidth = stitchedImage.getWidth() / numberOfSubImages;
        imageHeight = stitchedImage.getHeight();
        offsetX = indexImage * imageWidth;
        offsetY = 0;
    }
    else
    {
        imageWidth = stitchedImage.getWidth();
        imageHeight = stitchedImage.getHeight() / numberOfSubImages;
        offsetX = 0;
        offsetY = indexImage * imageHeight;
    }

    sourceX = (getWidth () - imageWidth) / 2;
    sourceY = (getHeight () - imageHeight) / 2;

    if (! sliderEnabled)
        g.setOpacity (0.6f);
    else
        g.setOpacity (1.0f);

    g.drawImage (stitchedImage,
                 sourceX, sourceY, imageWidth, imageHeight,
                 offsetX, offsetY, imageWidth, imageHeight);
}

END_JUCE_NAMESPACE
