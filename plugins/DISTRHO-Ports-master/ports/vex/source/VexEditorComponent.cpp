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

#include "VexEditorComponent.h"
#include "vex/VexWaveRenderer.h"

VexEditorComponent::VexEditorComponent(AudioProcessor* const proc, Callback* const callback, VexArpSettings& arpSet1, VexArpSettings& arpSet2, VexArpSettings& arpSet3)
    : AudioProcessorEditor(proc),
      fCallback(callback),
      fNeedsUpdate(false)
{
    internalCachedImage1 = ImageCache::getFromMemory(Resources::vex3_png, Resources::vex3_pngSize);

    // Comboboxes, wave selection
    addAndMakeVisible(comboBox1 = new ComboBox("comboBox1"));
    comboBox1->setEditableText(false);
    comboBox1->setJustificationType(Justification::centredLeft);
    comboBox1->setTextWhenNothingSelected(String("silent"));
    comboBox1->setTextWhenNoChoicesAvailable(String("silent"));
    comboBox1->addListener(this);
    comboBox1->setColour(ComboBox::backgroundColourId, Colours::black);
    comboBox1->setColour(ComboBox::textColourId, Colours::lightgrey);
    comboBox1->setColour(ComboBox::outlineColourId, Colours::grey);
    comboBox1->setColour(ComboBox::buttonColourId, Colours::grey);
    comboBox1->setWantsKeyboardFocus(false);
    comboBox1->setLookAndFeel(&mlaf);

    addAndMakeVisible(comboBox2 = new ComboBox("comboBox2"));
    comboBox2->setEditableText(false);
    comboBox2->setJustificationType(Justification::centredLeft);
    comboBox2->setTextWhenNothingSelected(String("silent"));
    comboBox2->setTextWhenNoChoicesAvailable(String("silent"));
    comboBox2->addListener(this);
    comboBox2->setColour(ComboBox::backgroundColourId, Colours::black);
    comboBox2->setColour(ComboBox::textColourId, Colours::lightgrey);
    comboBox2->setColour(ComboBox::outlineColourId, Colours::grey);
    comboBox2->setColour(ComboBox::buttonColourId, Colours::grey);
    comboBox2->setWantsKeyboardFocus(false);
    comboBox2->setLookAndFeel(&mlaf);

    addAndMakeVisible(comboBox3 = new ComboBox("comboBox3"));
    comboBox3->setEditableText(false);
    comboBox3->setJustificationType(Justification::centredLeft);
    comboBox3->setTextWhenNothingSelected(String("silent"));
    comboBox3->setTextWhenNoChoicesAvailable(String("silent"));
    comboBox3->addListener(this);
    comboBox3->setColour(ComboBox::backgroundColourId, Colours::black);
    comboBox3->setColour(ComboBox::textColourId, Colours::lightgrey);
    comboBox3->setColour(ComboBox::outlineColourId, Colours::grey);
    comboBox3->setColour(ComboBox::buttonColourId, Colours::grey);
    comboBox3->setWantsKeyboardFocus(false);
    comboBox3->setLookAndFeel(&mlaf);

    for (int i = 0, tableSize = WaveRenderer::getWaveTableSize(); i < tableSize; ++i)
    {
        String tableName(WaveRenderer::getWaveTableName(i));

        comboBox1->addItem(tableName, i + 1);
        comboBox2->addItem(tableName, i + 1);
        comboBox3->addItem(tableName, i + 1);
    }

    //Make sliders
    for (int i = 0; i < kSliderCount; ++i)
    {
        addAndMakeVisible(sliders[i] = new SnappingSlider("s"));
        sliders[i]->setSliderStyle(Slider::RotaryVerticalDrag);
        sliders[i]->setRange(0, 1, 0);
        sliders[i]->setSnap(0.0f, 0.0f);
        sliders[i]->setLookAndFeel(&mlaf);
        sliders[i]->addListener(this);
    }

    //Adjust the center of some
    sliders[1]->setRange(0, 1, 0.25f);
    sliders[1]->setSnap(0.5f, 0.05f);
    sliders[2]->setSnap(0.5f, 0.05f);
    sliders[3]->setSnap(0.5f, 0.05f);
    sliders[4]->setSnap(0.5f, 0.05f);
    sliders[8]->setSnap(0.5f, 0.05f);
    sliders[13]->setSnap(0.5f, 0.05f);
    sliders[18]->setSnap(0.5f, 0.05f);

    int tmp = 24;
    sliders[1 + tmp]->setRange(0, 1, 0.25f);
    sliders[1 + tmp]->setSnap(0.5f, 0.05f);
    sliders[2 + tmp]->setSnap(0.5f, 0.05f);
    sliders[3 + tmp]->setSnap(0.5f, 0.05f);
    sliders[4 + tmp]->setSnap(0.5f, 0.05f);
    sliders[8 + tmp]->setSnap(0.5f, 0.05f);
    sliders[13 + tmp]->setSnap(0.5f, 0.05f);
    sliders[18 + tmp]->setSnap(0.5f, 0.05f);

    tmp = 48;
    sliders[1 + tmp]->setRange(0, 1, 0.25f);
    sliders[1 + tmp]->setSnap(0.5f, 0.05f);
    sliders[2 + tmp]->setSnap(0.5f, 0.05f);
    sliders[3 + tmp]->setSnap(0.5f, 0.05f);
    sliders[4 + tmp]->setSnap(0.5f, 0.05f);
    sliders[8 + tmp]->setSnap(0.5f, 0.05f);
    sliders[13 + tmp]->setSnap(0.5f, 0.05f);
    sliders[18 + tmp]->setSnap(0.5f, 0.05f);

    sliders[83]->setSnap(0.5f, 0.05f);
    sliders[84]->setSnap(0.5f, 0.05f);
    sliders[85]->setSnap(0.5f, 0.05f);

    sliders[75]->setSnap(0.0f, 0.05f);
    sliders[78]->setSnap(0.0f, 0.05f);
    sliders[82]->setSnap(0.0f, 0.05f);

    //PART ON/OFF
    addAndMakeVisible(TB1 = new TextButton("TB1"));
    TB1->setButtonText(String());
    TB1->addListener(this);
    TB1->setColour(TextButton::buttonColourId, Colours::darkred.withAlpha(0.5f));
    TB1->setColour(TextButton::buttonOnColourId, Colours::red);
    TB1->setClickingTogglesState(true);
    TB1->setToggleState(false, dontSendNotification);

    addAndMakeVisible(TB2 = new TextButton("TB2"));
    TB2->setButtonText(String());
    TB2->addListener(this);
    TB2->setColour(TextButton::buttonColourId, Colours::darkred.withAlpha(0.5f));
    TB2->setColour(TextButton::buttonOnColourId, Colours::red);
    TB2->setClickingTogglesState(true);
    TB2->setToggleState(false, dontSendNotification);

    addAndMakeVisible(TB3 = new TextButton("TB3"));
    TB3->setButtonText(String());
    TB3->addListener(this);
    TB3->setColour(TextButton::buttonColourId, Colours::darkred.withAlpha(0.5f));
    TB3->setColour(TextButton::buttonOnColourId, Colours::red);
    TB3->setClickingTogglesState(true);
    TB3->setToggleState(false, dontSendNotification);

    //Peggy ON/OFF
    addAndMakeVisible(TB4 = new TextButton("TB4"));
    TB4->setButtonText(String());
    TB4->addListener(this);
    TB4->setColour(TextButton::buttonColourId, Colours::darkblue.withAlpha(0.5f));
    TB4->setColour(TextButton::buttonOnColourId, Colours::blue);
    TB4->setClickingTogglesState(true);
    TB4->setToggleState(false, dontSendNotification);

    addAndMakeVisible(TB5 = new TextButton("TB5"));
    TB5->setButtonText(String());
    TB5->addListener(this);
    TB5->setColour(TextButton::buttonColourId, Colours::darkblue.withAlpha(0.5f));
    TB5->setColour(TextButton::buttonOnColourId, Colours::blue);
    TB5->setClickingTogglesState(true);
    TB5->setToggleState(false, dontSendNotification);

    addAndMakeVisible(TB6 = new TextButton("TB6"));
    TB6->setButtonText(String());
    TB6->addListener(this);
    TB6->setColour(TextButton::buttonColourId, Colours::darkblue.withAlpha(0.5f));
    TB6->setColour(TextButton::buttonOnColourId, Colours::blue);
    TB6->setClickingTogglesState(true);
    TB6->setToggleState(false, dontSendNotification);

    addChildComponent(p1 = new PeggyViewComponent(arpSet1, this));
    p1->setLookAndFeel(&mlaf);
    addChildComponent(p2 = new PeggyViewComponent(arpSet2, this));
    p2->setLookAndFeel(&mlaf);
    addChildComponent(p3 = new PeggyViewComponent(arpSet3, this));
    p3->setLookAndFeel(&mlaf);

    setSize(800,500);
    updateParametersFromFilter(true);

    startTimer(50);
}

VexEditorComponent::~VexEditorComponent()
{
    stopTimer();
    removeAllChildren();
}

void VexEditorComponent::paint(Graphics& g)
{
    g.drawImage(internalCachedImage1,
                0, 0, 800, 500,
                0, 0, internalCachedImage1.getWidth(), internalCachedImage1.getHeight());
}

void VexEditorComponent::resized()
{
    comboBox1->setBounds(13, 38, 173, 23);
    comboBox2->setBounds(213, 38, 173, 23);
    comboBox3->setBounds(413, 38, 173, 23);

    // ...
    sliders[73]->setBounds(623, 23, 28, 28);
    sliders[74]->setBounds(686, 23, 28, 28);
    sliders[75]->setBounds(748, 23, 28, 28);
    sliders[76]->setBounds(623, 98, 28, 28);
    sliders[77]->setBounds(686, 98, 28, 28);
    sliders[78]->setBounds(748, 98, 28, 28);
    sliders[79]->setBounds(611, 173, 28, 28);
    sliders[80]->setBounds(661, 173, 28, 28);
    sliders[81]->setBounds(711, 173, 28, 28);
    sliders[82]->setBounds(761, 173, 28, 28);
    sliders[85]->setBounds(711, 373, 28, 28);
    sliders[84]->setBounds(661, 373, 28, 28);
    sliders[83]->setBounds(611, 373, 28, 28);
    sliders[86]->setBounds(611, 449, 28, 28);
    sliders[87]->setBounds(661, 449, 28, 28);
    sliders[88]->setBounds(711, 449, 28, 28);
    sliders[0]->setBounds(761, 449, 28, 28);

    // ...
    sliders[1]->setBounds(11, 73, 28, 28);
    sliders[2]->setBounds(61, 73, 28, 28);
    sliders[3]->setBounds(111, 73, 28, 28);
    sliders[4]->setBounds(161, 73, 28, 28);
    sliders[24 + 1]->setBounds(211, 73, 28, 28);
    sliders[24 + 2]->setBounds(261, 73, 28, 28);
    sliders[24 + 3]->setBounds(311, 73, 28, 28);
    sliders[24 + 4]->setBounds(361, 73, 28, 28);
    sliders[48 + 1]->setBounds(411, 73, 28, 28);
    sliders[48 + 2]->setBounds(461, 73, 28, 28);
    sliders[48 + 3]->setBounds(511, 73, 28, 28);
    sliders[48 + 4]->setBounds(561, 73, 28, 28);

    // ...
    sliders[5]->setBounds(11, 149, 28, 28);
    sliders[6]->setBounds(61, 149, 28, 28);
    sliders[7]->setBounds(111, 149, 28, 28);
    sliders[8]->setBounds(161, 149, 28, 28);
    sliders[24 + 5]->setBounds(211, 149, 28, 28);
    sliders[24 + 6]->setBounds(261, 149, 28, 28);
    sliders[24 + 7]->setBounds(311, 149, 28, 28);
    sliders[24 + 8]->setBounds(361, 149, 28, 28);
    sliders[48 + 5]->setBounds(411, 149, 28, 28);
    sliders[48 + 6]->setBounds(461, 149, 28, 28);
    sliders[48 + 7]->setBounds(511, 149, 28, 28);
    sliders[48 + 8]->setBounds(561, 149, 28, 28);

    // ...
    sliders[9]->setBounds(11, 223, 28, 28);
    sliders[10]->setBounds(48, 223, 28, 28);
    sliders[11]->setBounds(86, 223, 28, 28);
    sliders[12]->setBounds(123, 223, 28, 28);
    sliders[13]->setBounds(161, 223, 28, 28);
    sliders[24 + 9]->setBounds(211, 223, 28, 28);
    sliders[24 + 10]->setBounds(248, 223, 28, 28);
    sliders[24 + 11]->setBounds(286, 223, 28, 28);
    sliders[24 + 12]->setBounds(323, 223, 28, 28);
    sliders[24 + 13]->setBounds(361, 223, 28, 28);
    sliders[48 + 9]->setBounds(411, 223, 28, 28);
    sliders[48 + 10]->setBounds(448, 223, 28, 28);
    sliders[48 + 11]->setBounds(486, 223, 28, 28);
    sliders[48 + 12]->setBounds(523, 223, 28, 28);
    sliders[48 + 13]->setBounds(561, 223, 28, 28);

    // ...
    sliders[14]->setBounds(11, 298, 28, 28);
    sliders[15]->setBounds(48, 298, 28, 28);
    sliders[16]->setBounds(86, 298, 28, 28);
    sliders[17]->setBounds(123, 298, 28, 28);
    sliders[18]->setBounds(161, 298, 28, 28);
    sliders[24 + 14]->setBounds(211, 298, 28, 28);
    sliders[24 + 15]->setBounds(248, 298, 28, 28);
    sliders[24 + 16]->setBounds(286, 298, 28, 28);
    sliders[24 + 17]->setBounds(323, 298, 28, 28);
    sliders[24 + 18]->setBounds(361, 298, 28, 28);
    sliders[48 + 14]->setBounds(411, 298, 28, 28);
    sliders[48 + 15]->setBounds(448, 298, 28, 28);
    sliders[48 + 16]->setBounds(486, 298, 28, 28);
    sliders[48 + 17]->setBounds(523, 298, 28, 28);
    sliders[48 + 18]->setBounds(561, 298, 28, 28);

    // ...
    sliders[19]->setBounds(11, 374, 28, 28);
    sliders[20]->setBounds(86, 374, 28, 28);
    sliders[21]->setBounds(161, 374, 28, 28);
    sliders[24 + 19]->setBounds(211, 374, 28, 28);
    sliders[24 + 20]->setBounds(286, 374, 28, 28);
    sliders[24 + 21]->setBounds(361, 374, 28, 28);
    sliders[48 + 19]->setBounds(411, 374, 28, 28);
    sliders[48 + 20]->setBounds(486, 374, 28, 28);
    sliders[48 + 21]->setBounds(561, 374, 28, 28);

    // ...
    sliders[22]->setBounds(11, 448, 28, 28);
    sliders[23]->setBounds(86, 448, 28, 28);
    sliders[24]->setBounds(161, 448, 28, 28);
    sliders[24 + 22]->setBounds(211, 448, 28, 28);
    sliders[24 + 23]->setBounds(286, 448, 28, 28);
    sliders[48]->setBounds(361, 448, 28, 28);
    sliders[48 + 22]->setBounds(411, 448, 28, 28);
    sliders[48 + 23]->setBounds(486, 448, 28, 28);
    sliders[48 + 24]->setBounds(561, 448, 28, 28);

    TB1->setBounds(174, 4, 13, 13);
    TB2->setBounds(374, 4, 13, 13);
    TB3->setBounds(574, 4, 13, 13);
    TB4->setBounds(154, 4, 13, 13);
    TB5->setBounds(354, 4, 13, 13);
    TB6->setBounds(554, 4, 13, 13);

    p1->setBounds(10, 20, 207, 280);
    p2->setBounds(210, 20, 207, 280);
    p3->setBounds(410, 20, 207, 280);
}

void VexEditorComponent::timerCallback()
{
    if (fNeedsUpdate)
    {
        updateParametersFromFilter(true);
        fNeedsUpdate = false;
    }
    else
    {
        updateParametersFromFilter(false);
    }
}

void VexEditorComponent::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == comboBox1)
        fCallback->editorWaveChanged(1, comboBox1->getText());
    else if (comboBoxThatHasChanged == comboBox2)
        fCallback->editorWaveChanged(2, comboBox2->getText());
    else if (comboBoxThatHasChanged == comboBox3)
        fCallback->editorWaveChanged(3, comboBox3->getText());
}

void VexEditorComponent::sliderValueChanged(Slider* sliderThatWasMoved)
{
    for (int i = 0; i < kSliderCount; ++i)
    {
        if (sliders[i] == sliderThatWasMoved)
        {
            fCallback->editorParameterChanged(i, (float)sliderThatWasMoved->getValue());
            return;
        }
    }
}

void VexEditorComponent::buttonClicked(Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == TB1)
        fCallback->editorParameterChanged(89, TB1->getToggleState() ? 1.0f : 0.0f);
    else if (buttonThatWasClicked == TB2)
        fCallback->editorParameterChanged(90, TB2->getToggleState() ? 1.0f : 0.0f);
    else if (buttonThatWasClicked == TB3)
        fCallback->editorParameterChanged(91, TB3->getToggleState() ? 1.0f : 0.0f);
    else if (buttonThatWasClicked == TB4)
        p1->setVisible(!p1->isVisible());
    else if (buttonThatWasClicked == TB5)
        p2->setVisible(!p2->isVisible());
    else if (buttonThatWasClicked == TB6)
        p3->setVisible(!p3->isVisible());
}

void VexEditorComponent::updateParametersFromFilter(const bool all)
{
    if (all)
    {
        for (int i = 0; i < kSliderCount; ++i)
            sliders[i]->setValue(fCallback->getFilterParameterValue(i), dontSendNotification);

        TB1->setToggleState(fCallback->getFilterParameterValue(89) > 0.5f ? true : false, dontSendNotification);
        TB2->setToggleState(fCallback->getFilterParameterValue(90) > 0.5f ? true : false, dontSendNotification);
        TB3->setToggleState(fCallback->getFilterParameterValue(91) > 0.5f ? true : false, dontSendNotification);

        comboBox1->setText(fCallback->getFilterWaveName(1), dontSendNotification);
        comboBox2->setText(fCallback->getFilterWaveName(2), dontSendNotification);
        comboBox3->setText(fCallback->getFilterWaveName(3), dontSendNotification);

        p1->update();
        p2->update();
        p3->update();
    }
    else
    {
        bool params[92];
        fCallback->getChangedParameters(params);

        for (int i = 0; i < kSliderCount; ++i)
        {
            if (params[i])
                sliders[i]->setValue(fCallback->getFilterParameterValue(i), dontSendNotification);
        }

        if (params[89]) TB1->setToggleState(fCallback->getFilterParameterValue(89) > 0.5f ? true : false, dontSendNotification);
        if (params[90]) TB2->setToggleState(fCallback->getFilterParameterValue(90) > 0.5f ? true : false, dontSendNotification);
        if (params[91]) TB3->setToggleState(fCallback->getFilterParameterValue(91) > 0.5f ? true : false, dontSendNotification);
    }
}
