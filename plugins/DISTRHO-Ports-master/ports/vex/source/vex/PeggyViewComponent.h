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

#ifndef DISTRHO_VEX_PEGGYVIEWCOMPONENT_HEADER_INCLUDED
#define DISTRHO_VEX_PEGGYVIEWCOMPONENT_HEADER_INCLUDED

#include "VexArpSettings.h"

#include "gui/BoolGridComponent.h"
#include "gui/SliderFieldComponent.h"
#include "lookandfeel/MyLookAndFeel.h"

class PeggyViewComponent : public Component,
                           public ChangeListener,
                           public SliderListener,
                           public ComboBoxListener,
                           public ButtonListener
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void arpParameterChanged(const uint32_t id) = 0;
    };

    PeggyViewComponent(VexArpSettings& arpSet, Callback* const callback)
        : fArpSettings(arpSet),
          fCallback(callback)
    {
        addAndMakeVisible(boolGrid = new BoolGridComponent());
        boolGrid->addChangeListener(this);

        addAndMakeVisible(sliderField = new SliderFieldComponent());
        sliderField->addChangeListener(this);

        addAndMakeVisible(length = new Slider("sdf"));
        length->setRange(1, 16, 1);
        length->setTextBoxStyle(Slider::NoTextBox, true, 0, 0 );
        length->addListener(this);

        addAndMakeVisible(timeMode = new ComboBox("a"));
        timeMode->setEditableText (false);
        timeMode->setJustificationType (Justification::centredLeft);
        timeMode->setTextWhenNothingSelected (String("_"));
        timeMode->setTextWhenNoChoicesAvailable (String("_"));
        timeMode->setColour(ComboBox::backgroundColourId, Colours::black);
        timeMode->setColour(ComboBox::textColourId, Colours::lightgrey);
        timeMode->setColour(ComboBox::outlineColourId, Colours::grey);
        timeMode->setColour(ComboBox::buttonColourId, Colours::grey);
        timeMode->setWantsKeyboardFocus(false);
        timeMode->addItem("1/8",  1);
        timeMode->addItem("1/16", 2);
        timeMode->addItem("1/32", 3);
        timeMode->addListener(this);

        addAndMakeVisible(syncMode = new ComboBox("a"));
        syncMode->setEditableText (false);
        syncMode->setJustificationType (Justification::centredLeft);
        syncMode->setTextWhenNothingSelected (String("_"));
        syncMode->setTextWhenNoChoicesAvailable (String("_"));
        syncMode->setColour(ComboBox::backgroundColourId, Colours::black);
        syncMode->setColour(ComboBox::textColourId, Colours::lightgrey);
        syncMode->setColour(ComboBox::outlineColourId, Colours::grey);
        syncMode->setColour(ComboBox::buttonColourId, Colours::grey);
        syncMode->setWantsKeyboardFocus(false);
        syncMode->addItem("Key Sync", 1);
        syncMode->addItem("Bar Sync", 2);
        syncMode->addListener(this);

        addAndMakeVisible(failMode = new ComboBox("a"));
        failMode->setEditableText(false);
        failMode->setJustificationType (Justification::centredLeft);
        failMode->setTextWhenNothingSelected (String("_"));
        failMode->setTextWhenNoChoicesAvailable (String("_"));
        failMode->setColour(ComboBox::backgroundColourId, Colours::black);
        failMode->setColour(ComboBox::textColourId, Colours::lightgrey);
        failMode->setColour(ComboBox::outlineColourId, Colours::grey);
        failMode->setColour(ComboBox::buttonColourId, Colours::grey);
        failMode->setWantsKeyboardFocus(false);
        failMode->addItem("Silent Step", 1);
        failMode->addItem("Skip One", 2);
        failMode->addItem("Skip Two", 3);
        failMode->addListener(this);

        addAndMakeVisible(velMode = new ComboBox("a"));
        velMode->setEditableText(false);
        velMode->setJustificationType(Justification::centredLeft);
        velMode->setTextWhenNothingSelected (String("_"));
        velMode->setTextWhenNoChoicesAvailable (String("_"));
        velMode->setColour(ComboBox::backgroundColourId, Colours::black);
        velMode->setColour(ComboBox::textColourId, Colours::lightgrey);
        velMode->setColour(ComboBox::outlineColourId, Colours::grey);
        velMode->setColour(ComboBox::buttonColourId, Colours::grey);
        velMode->setWantsKeyboardFocus(false);
        velMode->addItem("Pattern Velocity", 1);
        velMode->addItem("Input Velocity", 2);
        velMode->addItem("Sum Velocities", 3);
        velMode->addListener(this);

        addAndMakeVisible(onOffBtn = new ToggleButton ("new button"));
        onOffBtn->setButtonText(String( "On"));
        onOffBtn->addListener(this);
        onOffBtn->setClickingTogglesState(true);

        update();
    }

    ~PeggyViewComponent() override
    {
        removeAllChildren();
    }

    void resized() override
    {
        boolGrid->setBounds(5, 5, 193, 63);
        sliderField->setBounds(5, 68, 193, 63);
        length->setBounds(4, 140, 194, 16);
        timeMode->setBounds(6, 160, 93, 23);
        syncMode->setBounds(102, 160, 93, 23);
        failMode->setBounds(6, 190, 189, 23);
        velMode-> setBounds(6, 220, 189, 23);
        onOffBtn->setBounds(6, 250, 60, 24);
    }

    void paint(Graphics& g) override
    {
        g.setColour(Colours::black.withAlpha(.5f));
        g.fillRect(5,5,getWidth(), getHeight());

        g.setGradientFill(ColourGradient(Colour(0xffffffff), 0.0f, 0.0f,
                          Colour(0xff888899), (float)getWidth(), (float)getHeight(), false));

        g.fillRect(0, 0, getWidth() - 5, getHeight() - 5);

        g.setColour(Colours::black);
        g.drawRect(0, 0, getWidth() - 5, getHeight() - 5);
    }

    void changeListenerCallback(ChangeBroadcaster* caller) override
    {
        if (caller == boolGrid)
        {
            int i = boolGrid->getLastChanged();
            fArpSettings.grid[i] = boolGrid->getCellState(i);
            fCallback->arpParameterChanged(6+VexArpSettings::kVelocitiesSize+i);
        }
        else if (caller == sliderField)
        {
            int i = sliderField->getLastSlider();
            fArpSettings.velocities[i] = jlimit(0.0f, 1.0f, sliderField->getValue(i));
            fCallback->arpParameterChanged(6+i);
        }
    }

    void comboBoxChanged(ComboBox* caller) override
    {
        if (caller == timeMode)
        {
            fArpSettings.timeMode = timeMode->getSelectedId();
            fCallback->arpParameterChanged(2);
        }
        else if (caller == syncMode)
        {
            fArpSettings.syncMode = syncMode->getSelectedId();
            fCallback->arpParameterChanged(3);
        }
        else if (caller == failMode)
        {
            fArpSettings.failMode = failMode->getSelectedId();
            fCallback->arpParameterChanged(4);
        }
        else if (caller == velMode)
        {
            fArpSettings.velMode = velMode->getSelectedId();
            fCallback->arpParameterChanged(5);
        }
    }

    void sliderValueChanged(Slider* /*caller*/) override
    {
        boolGrid->setLength((int)length->getValue());
        sliderField->setLength((int)length->getValue());
        fArpSettings.length = (int)length->getValue();
        fCallback->arpParameterChanged(1);
    }

    void buttonClicked(Button* /*caller*/) override
    {
        fArpSettings.on = onOffBtn->getToggleState();
        fCallback->arpParameterChanged(0);
    }

    void update()
    {
        onOffBtn->setToggleState(fArpSettings.on, dontSendNotification);

        sliderField->setLength(fArpSettings.length);
        boolGrid->setLength(fArpSettings.length);
        length->setValue(fArpSettings.length);

        for (int i = 0; i < VexArpSettings::kVelocitiesSize; ++i)
            sliderField->setValue(i, fArpSettings.velocities[i]);

        for (int i = 0; i < VexArpSettings::kGridSize; ++i)
            boolGrid->setCellState(i, fArpSettings.grid[i]);

        timeMode->setSelectedId(fArpSettings.timeMode);
        syncMode->setSelectedId(fArpSettings.syncMode);
        failMode->setSelectedId(fArpSettings.failMode);
        velMode->setSelectedId(fArpSettings.velMode);
    }

private:
    VexArpSettings& fArpSettings;
    Callback* const fCallback;

    ScopedPointer<BoolGridComponent> boolGrid;
    ScopedPointer<SliderFieldComponent> sliderField;

    ScopedPointer<ComboBox> timeMode;
    ScopedPointer<ComboBox> syncMode;
    ScopedPointer<ComboBox> failMode;
    ScopedPointer<ComboBox> velMode;

    ScopedPointer<ToggleButton> onOffBtn;
    ScopedPointer<Slider> length;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PeggyViewComponent)
};

#endif // DISTRHO_VEX_PEGGYVIEWCOMPONENT_HEADER_INCLUDED
