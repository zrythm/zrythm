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
JuceticeLookAndFeel::JuceticeLookAndFeel()
{
/*
    setColour (ComboBox::buttonColourId,                    Colour (0xffbbbbff));
    setColour (ComboBox::outlineColourId,                   Colours::grey.withAlpha (0.7f));

    setColour (ListBox::outlineColourId,            findColour (ComboBox::outlineColourId));

    setColour (ScrollBar::backgroundColourId,       Colours::transparentBlack);
    setColour (ScrollBar::thumbColourId,            Colours::white);

    setColour (Slider::thumbColourId,               findColour (TextButton::buttonColourId));
    setColour (Slider::trackColourId,               Colour (0x7fffffff));
    setColour (Slider::textBoxOutlineColourId,      findColour (ComboBox::outlineColourId));

    setColour (ProgressBar::backgroundColourId,     Colours::white);
    setColour (ProgressBar::foregroundColourId,     Colour (0xffaaaaee));

    setColour (PopupMenu::backgroundColourId,             Colours::white);
    setColour (PopupMenu::highlightedTextColourId,        Colours::white);
    setColour (PopupMenu::highlightedBackgroundColourId,  Colour (0x991111aa));

    setColour (TextEditor::focusedOutlineColourId,  findColour (TextButton::buttonColourId));
*/
}

JuceticeLookAndFeel::~JuceticeLookAndFeel()
{
}

//==============================================================================
Font JuceticeLookAndFeel::getPopupMenuFont()
{
    return Font (14.0f);
}

//==============================================================================
void JuceticeLookAndFeel::drawToggleButton (Graphics& g,
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
    g.setFont (jmin (15.0f, button.getHeight() * 0.6f));

    if (! button.isEnabled())
        g.setOpacity (0.5f);

    const int textX = tickWidth + 5;

    g.drawFittedText (button.getButtonText(),
                      textX, 4,
                      button.getWidth() - textX - 2, button.getHeight() - 8,
                      Justification::centredLeft, 10);
}

//==============================================================================
/*
void JuceticeLookAndFeel::drawGroupComponentOutline (Graphics& g, int width, int height,
                                                     const String& text,
                                                     const Justification& position,
                                                     GroupComponent& group)
{
    const float textH = 14.0f;
    const float indent = 0.0f;
    const float textEdgeGap = 2.0f;
    float cs = 5.0f;

    Font f (textH);

    float x = indent;
    float y = f.getAscent() - 3.0f;
    float w = jmax (0.0f, width - x * 2.0f);
    float h = jmax (0.0f, height - y  - indent);
    cs = jmin (cs, w * 0.5f, h * 0.5f);
    const float cs2 = 2.0f * cs;

    float textW = jlimit (0.0f, jmax (0.0f, w - cs2 - textEdgeGap * 2), f.getStringWidth (text) + textEdgeGap * 2.0f);
    float textX = cs + textEdgeGap;

    if (position.testFlags (Justification::horizontallyCentred))
        textX = cs + (w - cs2 - textW) * 0.5f;
    else if (position.testFlags (Justification::right))
        textX = w - cs - textW - textEdgeGap;

    const float alpha = group.isEnabled() ? 1.0f : 0.5f;

    // draw main background
    g.setColour (group.findColour (GroupComponent::outlineColourId)
                    .withMultipliedAlpha (alpha));

    g.fillRoundedRectangle (x, y, w, h, 4.0f);

    // draw title background
    g.setColour (group.findColour (GroupComponent::textColourId)
                    .withMultipliedAlpha (alpha));

    g.fillRoundedRectangle (x, y, w, textH + 2, 4.0f);

    // draw title
    g.setColour (group.findColour (GroupComponent::textColourId).contrasting()
                    .withMultipliedAlpha (alpha));

    g.setFont (f);
    g.drawText (text,
                roundFloatToInt (x + textX), y,
                roundFloatToInt (textW),
                roundFloatToInt (textH),
                Justification::centred, true);
}
*/

END_JUCE_NAMESPACE
