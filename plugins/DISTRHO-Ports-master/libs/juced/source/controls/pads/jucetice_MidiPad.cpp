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
MidiPad::MidiPad ()
    : isPlaying (false),
      drawableButton (0)
{
    setMouseClickGrabsKeyboardFocus (false);

    addAndMakeVisible (drawableButton = new DrawablePad("MidiPad"));
    drawableButton->addListener (this);

    setSize (32, 16);
}

MidiPad::~MidiPad()
{
    deleteAndZero (drawableButton);
}

//==============================================================================
void MidiPad::paint (Graphics& g)
{
}

//==============================================================================
void MidiPad::resized()
{
    drawableButton->setBounds (0, 0, proportionOfWidth (1.0000f), proportionOfHeight (1.0000f));
}

//==============================================================================
void MidiPad::buttonClicked(Button* buttonThatWasClicked)
{
}

//==============================================================================
bool MidiPad::hitTest (int x, int y)
{
    if (drawableButton->isHex())
    {
        float xnorm = (float)x/(float)getWidth();
        float ynorm = 1-(float)y/(float)getHeight();
        if (xnorm<0.25)
        {
            if (ynorm<(0.5-2*xnorm)) return false;
            else if (ynorm>(0.5+2*xnorm)) return false;
            else return true;
        }
        else if (xnorm>0.75)
        {
            if (ynorm>(2.5-2*xnorm)) return false;
            else if (ynorm<(-1.5+2*xnorm)) return false;
            else return true;
        }
        else return true;
    }
    else return true;
}

//==============================================================================
bool MidiPad::isInterestedInFileDrag (const StringArray& files)
{
    File file = File(files.joinIntoString(String(),0,1));
    if (file.hasFileExtension("png") ||
        file.hasFileExtension("gif") ||
        file.hasFileExtension("jpg") ||
        file.hasFileExtension("svg") )
        return true;
    else return false;
}

//==============================================================================
void MidiPad::filesDropped(const juce::StringArray &filenames, int mouseX, int mouseY)
{
    if (isInterestedInFileDrag(filenames)) {
        String filename = filenames.joinIntoString(String(),0,1);
        File file = File(filename);
        Drawable* image = Drawable::createFromImageFile(file);
        drawableButton->setImages(image);
        //save the relative path
        drawableButton->setName(file.getRelativePathFrom(File::getSpecialLocation(File::currentExecutableFile)));
    }
}

//==============================================================================
void MidiPad::setButtonText (const String &newText) {
    drawableButton->setTooltip (newText);
}

//==============================================================================
void MidiPad::setColour (const juce::Colour &colour) {
    drawableButton->setBackgroundColours (colour, colour);
}

//==============================================================================
void MidiPad::setTriggeredOnMouseDown (const bool isTriggeredOnMouseDown) {
    drawableButton->setTriggeredOnMouseDown (isTriggeredOnMouseDown);
}

//==============================================================================
void MidiPad::addButtonListener (ButtonListener *const newListener) {
    drawableButton->addListener (newListener);
}

//==============================================================================
void MidiPad::setToggleState (bool state) {
    drawableButton->setToggleState (state, dontSendNotification);
}


END_JUCE_NAMESPACE
