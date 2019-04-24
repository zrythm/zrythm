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

 ==============================================================================
*/

#include "MyLookAndFeel.h"

MyLookAndFeel::MyLookAndFeel()
{
    MemoryInputStream fontStream (Resources::t_bin, Resources::t_binSize, false);
    CustomTypeface* tf = new CustomTypeface (fontStream);
    Topaz = new Font (tf);
//  delete tf;

    Topaz->setHeight (9.0f);
    Topaz->setHorizontalScale (1.0f);

//	Topaz = new Font (Font::getDefaultMonospacedFontName (), 12.0f, Font::plain);
//	Topaz->setHorizontalScale (1.0f);
}

MyLookAndFeel::~MyLookAndFeel()
{
    delete Topaz;
}

Font MyLookAndFeel::getComboBoxFont(ComboBox&)
{
    return *Topaz;
}

Font MyLookAndFeel::getPopupMenuFont()
{
    return *Topaz;
}

void MyLookAndFeel::getIdealPopupMenuItemSize(const String& text,
                                              const bool isSeparator,
                                              int standardMenuItemHeight,
                                              int& idealWidth,
                                              int& idealHeight)
{
    if (isSeparator)
    {
        idealWidth = 50;
        idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight / 2 : 10;
    }
    else
    {
        Font font (getPopupMenuFont());

        idealHeight = roundFloatToInt (font.getHeight() * 1.3f);
        idealWidth = font.getStringWidth (text) + idealHeight * 2;
    }
}


void MyLookAndFeel::drawToggleButton(Graphics& g,
                                     ToggleButton& button,
                                     bool isMouseOverButton,
                                     bool isButtonDown)
{
    const int tickWidth = jmin (20, button.getHeight() - 4);

    drawTickBox (g, button, 4, (button.getHeight() - tickWidth) / 2,
                 tickWidth, tickWidth,
                 button.getToggleState(),
                 button.isEnabled(),
                 isMouseOverButton,
                 isButtonDown);

    g.setColour (button.findColour (ToggleButton::textColourId));
    g.setFont (*Topaz);

    const int textX = tickWidth + 5;

    g.drawFittedText (button.getButtonText(),
                      textX, 4,
                      button.getWidth() - textX - 2, button.getHeight() - 8,
                      Justification::centredLeft, 10);
}

void MyLookAndFeel::drawRotarySlider(Graphics& g,
                                     int x, int y,
                                     int width, int height,
                                     float sliderPos,
                                     const float rotaryStartAngle,
                                     const float rotaryEndAngle,
                                     Slider& /*slider*/)
{
    const float radius = jmin (width / 2, height / 2) - 2.0f;
    const float centreX = x + width * 0.5f;
    const float centreY = y + height * 0.5f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float rw = radius * 2.0f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    //const bool isMouseOver = slider.isMouseOverOrDragging() && slider.isEnabled();
    //const float zeroPos = rotaryStartAngle + std::abs((float)slider.getMinimum() / ((float)slider.getMaximum() - (float)slider.getMinimum())) * (rotaryEndAngle - rotaryStartAngle);

    Path p;

    PathStrokeType (rw * 0.01f).createStrokedPath (p, p);

    p.addLineSegment (Line<float>(0.0f, -radius * 0.5f, 0.00f, -radius), rw * 0.08f);

    g.setColour (Colours::white.withAlpha (1.0f));

    g.fillPath (p, AffineTransform::rotation (angle).translated (centreX, centreY));

    g.setColour (Colours::black.withAlpha (0.7f));
    g.drawEllipse (rx, ry, rw, rw, 1.0f);
}
