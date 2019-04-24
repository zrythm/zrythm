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

#ifndef __JUCETICE_VEXPLUGINEDITOR_HEADER__
#define __JUCETICE_VEXPLUGINEDITOR_HEADER__

#include "vex/PeggyViewComponent.h"
#include "vex/gui/SnappingSlider.h"
#include "vex/lookandfeel/MyLookAndFeel.h"
#include "vex/resources/Resources.h"

class VexEditorComponent : public AudioProcessorEditor,
                           public Timer,
                           public ComboBox::Listener,
                           public Slider::Listener,
                           public Button::Listener,
                           public PeggyViewComponent::Callback // ignored
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void getChangedParameters(bool params[92]) = 0;
        virtual float getFilterParameterValue(const uint32_t index) const = 0;
        virtual String getFilterWaveName(const int part) const = 0;
        virtual void editorParameterChanged(const uint32_t index, const float value) = 0;
        virtual void editorWaveChanged(const int part, const String& wave) = 0;
    };

    VexEditorComponent(AudioProcessor* const proc, Callback* const callback, VexArpSettings& arpSet1, VexArpSettings& arpSet2, VexArpSettings& arpSet3);
    ~VexEditorComponent() override;

    void setNeedsUpdate()
    {
        fNeedsUpdate = true;
    }

protected:
    void paint (Graphics& g) override;
    void resized() override;

    void timerCallback() override;
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;
    void buttonClicked (Button* buttonThatWasClicked) override;

private:
    void arpParameterChanged(const uint32_t) override {}

    void updateParametersFromFilter(bool all);

    static const int kSliderCount = 89;

    Callback* const fCallback;
    volatile bool fNeedsUpdate;

    Image internalCachedImage1;
    MyLookAndFeel mlaf;

    ScopedPointer<ComboBox> comboBox1;
    ScopedPointer<ComboBox> comboBox2;
    ScopedPointer<ComboBox> comboBox3;

    ScopedPointer<SnappingSlider> sliders[kSliderCount];

    ScopedPointer<TextButton> TB1;
    ScopedPointer<TextButton> TB2;
    ScopedPointer<TextButton> TB3;
    ScopedPointer<TextButton> TB4;
    ScopedPointer<TextButton> TB5;
    ScopedPointer<TextButton> TB6;

    ScopedPointer<PeggyViewComponent> p1;
    ScopedPointer<PeggyViewComponent> p2;
    ScopedPointer<PeggyViewComponent> p3;
};

#endif
