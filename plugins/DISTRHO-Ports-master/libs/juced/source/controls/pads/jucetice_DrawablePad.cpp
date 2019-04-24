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

    Original code: Reuben a.k.a. Insertpizhere

 ==============================================================================
*/

BEGIN_JUCE_NAMESPACE

//==============================================================================
DrawablePad::DrawablePad (const String& name)
    : Button (name),
      normalImage (0),
      overImage (0),
      downImage (0),
      disabledImage (0),
      normalImageOn (0),
      overImageOn (0),
      downImageOn (0),
      disabledImageOn (0)
{
    setSize (200, 200);
    backgroundOff = Colour (0xffbbbbff);
    backgroundOn = Colour (0xff3333ff);
    Label = "Pad";
    Description = "x:0 y:0";
    showdot = false;
    showx = false;
    showy = false;
    hex = false;
    x = 0;
    y = 0;
    roundness = 0.4f;

    setMouseClickGrabsKeyboardFocus (false);
}

DrawablePad::~DrawablePad()
{
    deleteImages();
}

//==============================================================================
void DrawablePad::deleteImages() 
{
    deleteAndZero (normalImage);
    deleteAndZero (overImage);
    deleteAndZero (downImage);
    deleteAndZero (disabledImage);
    deleteAndZero (normalImageOn);
    deleteAndZero (overImageOn);
    deleteAndZero (downImageOn);
    deleteAndZero (disabledImageOn);
}

//==============================================================================
void DrawablePad::setImages (const Drawable* normal,
                             const Drawable* over,
                             const Drawable* down,
                             const Drawable* disabled,
                             const Drawable* normalOn,
                             const Drawable* overOn,
                             const Drawable* downOn,
                             const Drawable* disabledOn)
{
    deleteImages();

//    jassert (normal != 0); // you really need to give it at least a normal image..

    if (normal != 0) normalImage = normal->createCopy();
    if (over != 0) overImage = over->createCopy();
    if (down != 0) downImage = down->createCopy();
    if (disabled != 0) disabledImage = disabled->createCopy();
    if (normalOn != 0) normalImageOn = normalOn->createCopy();
    if (overOn != 0) overImageOn = overOn->createCopy();
    if (downOn != 0) downImageOn = downOn->createCopy();
    if (disabledOn != 0) disabledImageOn = disabledOn->createCopy();

    repaint();
}

//==============================================================================
void DrawablePad::setBackgroundColours (const Colour& toggledOffColour,
                                        const Colour& toggledOnColour)
{
    if (backgroundOff != toggledOffColour)// || backgroundOn != toggledOnColour)
    {
        backgroundOff = toggledOffColour;
        backgroundOn = toggledOffColour.contrasting(0.5f).withMultipliedAlpha(2.0);

        repaint();
    }
}

const Colour& DrawablePad::getBackgroundColour() const throw()
{
    return getToggleState() ? backgroundOn
                            : backgroundOff;
}

//==============================================================================
void DrawablePad::drawButtonBackground (Graphics& g,
                                        Button& button,
                                        const Colour& backgroundColour,
                                        bool isMouseOverButton,
                                        bool isButtonDown)
{
    const int width = button.getWidth();
    const int height = button.getHeight();

    const float indent = 2.0f;
    const int cornerSize = jmin (roundFloatToInt (width * roundness),
                                 roundFloatToInt (height * roundness));

    Colour bc (backgroundColour);
    if (isMouseOverButton)
    {
        if (isButtonDown)
            bc = bc.brighter();
        else if (bc.getBrightness() > 0.5f)
            bc = bc.darker (0.1f);
        else
            bc = bc.brighter (0.1f);
    }

    g.setColour (bc);

    if (hex)
    {
        g.fillPath (hexpath);

        g.setColour (bc.contrasting().withAlpha ((isMouseOverButton) ? 0.6f : 0.4f));
        g.strokePath (hexpath, PathStrokeType ((isMouseOverButton) ? 2.0f : 1.4f));
    }
    else
    {
        Path p;
        p.addRoundedRectangle (indent, indent,
                               width - indent * 2.0f,
                               height - indent * 2.0f,
                               (float) cornerSize);
        g.fillPath (p);

        g.setColour (bc.contrasting().withAlpha ((isMouseOverButton) ? 0.6f : 0.4f));
        g.strokePath (p, PathStrokeType ((isMouseOverButton) ? 2.0f : 1.4f));
    }
}

//==============================================================================
void DrawablePad::paintButton (Graphics& g, bool isMouseOverButton, bool isButtonDown)
{
    const int insetX = getWidth() / 4;
    const int insetY = getHeight() / 4;

    Rectangle<float> imageSpace;
    imageSpace.setBounds (insetX, insetY, getWidth() - insetX * 2, getHeight() - insetY * 2);
 
    drawButtonBackground (g,
                          *this,
                          getBackgroundColour(),
                          isMouseOverButton,
                          isButtonDown);

    g.setOpacity (1.0f);

    const Drawable* imageToDraw = 0;

    if (isEnabled())
    {
        imageToDraw = getCurrentImage();
    }
    else
    {
        imageToDraw = getToggleState() ? disabledImageOn
                                       : disabledImage;

        if (imageToDraw == 0)
        {
            g.setOpacity (0.4f);
            imageToDraw = getNormalImage();
        }
    }

    if (imageToDraw != 0)
    {
        g.setImageResamplingQuality (Graphics::highResamplingQuality);

        imageToDraw->drawWithin (g,
                                 imageSpace,
                                 RectanglePlacement::centred,
                                 1.0f);
    }

    float fontsize = jmin ((float)(proportionOfWidth(0.2f)),(float)(proportionOfHeight(0.15f)));
    if (fontsize < 5.0) fontsize=5.0;

    g.setFont (Font (fontsize, Font::bold));
    g.setColour (getBackgroundColour().contrasting(0.8f));
    g.drawText (Label,
                proportionOfWidth (0.0447f),
                proportionOfHeight (0.0499f),
                proportionOfWidth (0.9137f),
                proportionOfHeight (0.1355f),
                Justification::centred, true);

    if (showdot && ! hex)
    {
        g.setFont (Font (fontsize*0.9f, Font::plain));

        String xy;
        if (showx && showy) xy = "x:" + String((int)(x*127.1)) + " y:" + String((int)(y*127.1));
        else if (showx)     xy = "x:" + String((int)(x*127.1));
        else if (showy)     xy = "y:" + String((int)(y*127.1));

        g.drawText (xy,
                    proportionOfWidth (0.0447f),
                    proportionOfHeight (0.8057f),
                    proportionOfWidth (0.9137f),
                    proportionOfHeight (0.1355f),
                    Justification::centred, true);

        float diameter = jmin ((float)(proportionOfHeight(0.125f)), (float)(proportionOfWidth(0.5f)));

        g.setColour (Colour (0x88faa52a));
        g.fillEllipse ((float)(proportionOfWidth(x)) - diameter*0.5f, 
                       (float)(proportionOfHeight(1.0f-y)) - diameter*0.5f, 
                       diameter, 
                       diameter);

        g.setColour (Colour (0x99a52a88));
        g.drawEllipse ((float)(proportionOfWidth(x)) - diameter*0.5f, 
                       (float)(proportionOfHeight(1.0f-y)) - diameter*0.5f, 
                       diameter, 
                       diameter, 
                       diameter*0.1f);
    }

}

//==============================================================================
void DrawablePad::resized()
{
    hexpath.clear();
    hexpath.startNewSubPath ((float) (proportionOfWidth (0.0100f)), (float) (proportionOfHeight (0.5000f)));
    hexpath.lineTo ((float) (proportionOfWidth (0.2500f)), (float) (proportionOfHeight (0.0100f)));
    hexpath.lineTo ((float) (proportionOfWidth (0.7500f)), (float) (proportionOfHeight (0.0100f)));
    hexpath.lineTo ((float) (proportionOfWidth (0.9900f)), (float) (proportionOfHeight (0.5000f)));
    hexpath.lineTo ((float) (proportionOfWidth (0.7500f)), (float) (proportionOfHeight (0.9900f)));
    hexpath.lineTo ((float) (proportionOfWidth (0.2500f)), (float) (proportionOfHeight (0.9900f)));
    hexpath.closeSubPath();
}

//==============================================================================
void DrawablePad::setHex (bool newhex)
{
    hex = newhex;
    
    resized ();
    repaint ();
}

bool DrawablePad::isHex() {
    return hex;
}

//==============================================================================
const Drawable* DrawablePad::getCurrentImage() const throw()
{
    if (isDown())
        return getDownImage();

    if (isOver())
        return getOverImage();

    return getNormalImage();
}

const Drawable* DrawablePad::getNormalImage() const throw()
{
    return (getToggleState() && normalImageOn != 0) ? normalImageOn
                                                    : normalImage;
}

const Drawable* DrawablePad::getOverImage() const throw()
{
    const Drawable* d = normalImage;

    if (getToggleState())
    {
        if (overImageOn != 0)
            d = overImageOn;
        else if (normalImageOn != 0)
            d = normalImageOn;
        else if (overImage != 0)
            d = overImage;
    }
    else
    {
        if (overImage != 0)
            d = overImage;
    }

    return d;
}

const Drawable* DrawablePad::getDownImage() const throw()
{
    const Drawable* d = normalImage;

    if (getToggleState())
    {
        if (downImageOn != 0) d = downImageOn;
        else if (overImageOn != 0) d = overImageOn;
        else if (normalImageOn != 0) d = normalImageOn;
        else if (downImage != 0) d = downImage;
        else d = getOverImage();
    }
    else
    {
        if (downImage != 0) d = downImage;
        else d = getOverImage();
    }

    return d;
}


END_JUCE_NAMESPACE
