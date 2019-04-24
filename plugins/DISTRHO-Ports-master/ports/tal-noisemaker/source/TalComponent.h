/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-7 by Raw Material Software ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the
   GNU General Public License, as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   JUCE is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with JUCE; if not, visit www.gnu.org/licenses or write to the
   Free Software Foundation, Inc., 59 Temple Place, Suite 330,
   Boston, MA 02111-1307 USA

  ------------------------------------------------------------------------------

   If you'd like to release a closed-source product which uses JUCE, commercial
   licenses are also available: visit www.rawmaterialsoftware.com/juce for
   more information.

  ==============================================================================
*/

#ifndef TALCOMPONENTEDITOR_H
#define TALCOMPONENTEDITOR_H

#include "TalCore.h"
#include "./Component/FilmStripKnob.h"
#include "./Component/TalComboBox.h"
#include "./Component/ImageSlider.h"
#include "./Component/ImageToggleButton.h"
#include "./Component/AccordeonTab.h"
#include "./Component/AccordeonTabContainer.h"
#include "./Component/LogoPanel.h"
#include "./EnvelopeEditor/EnvelopeEditorView.h"
#include "./Engine/AudioUtils.h"


//==============================================================================
/**
    This is the Component that our filter will use as its UI.

    One or more of these is created by the DemoJuceFilter::createEditor() method,
    and they will be deleted at some later time by the wrapper code.

    To demonstrate the correct way of connecting a filter to its UI, this
    class is a ChangeListener, and our demo filter is a ChangeBroadcaster. The
    editor component registers with the filter when it's created and deregisters
    when it's destroyed. When the filter's parameters are changed, it broadcasts
    a message and this editor responds by updating its display.
*/
class TalComponent   : public AudioProcessorEditor,
                          public ChangeListener,
                          public SliderListener,
						  public ButtonListener,
						  public ComboBoxListener
{
public:
    TalComponent(TalCore* const ownerFilter);
    ~TalComponent() override;

    //==============================================================================
    /** Our demo filter is a ChangeBroadcaster, and will call us back when one of
        its parameters changes.
    */
    void changeListenerCallback (ChangeBroadcaster* source) override;
    void sliderValueChanged (Slider*) override;

	void sliderDragStarted (Slider* slider) override;
	void sliderDragEnded (Slider* slider) override;

	void buttonClicked (Button *) override;
    void handleClickedTabs (Button* caller);

    void comboBoxChanged(ComboBox* comboBox) override;

    //==============================================================================
    /** Standard Juce paint callback. */
    void paint (Graphics& g) override;
    void resized() override;

    static const char* bmp00128_png;
    static const int bmp00128_pngSize;

    static const char* bmp00129_png;
    static const int bmp00129_pngSize;

    static const char* bmp00130_png;
    static const int bmp00130_pngSize;

    static const char* bmp00131_png;
    static const int bmp00131_pngSize;

    static const char* bmp00132_png;
    static const int bmp00132_pngSize;

    static const char* bmp00133_png;
    static const int bmp00133_pngSize;

    static const char* bmp00134_png;
    static const int bmp00134_pngSize;

    static const char* bmp00135_png;
    static const int bmp00135_pngSize;

    static const char* bmp00136_png;
    static const int bmp00136_pngSize;

    static const char* bmp00137_png;
    static const int bmp00137_pngSize;

private:
    AccordeonTabContainer *accordeonTabContainer;
    AccordeonTab *envelopeEditorAccordeonTab;
    AccordeonTab *synth1AccordeonTab;
    AccordeonTab *synth2AccordeonTab;
    AccordeonTab *controlAccordeonTab;
    LogoPanel *logoPanel;

    EnvelopeEditorView *envelopeEditorView;

	FilmStripKnob *volumeKnob;

    TalComboBox *filtertypeTalComboBox;
	FilmStripKnob *cutoffKnob;
	FilmStripKnob *resonanceKnob;
	FilmStripKnob *filterContourKnob;
	FilmStripKnob *keyfollowKnob;

	FilmStripKnob *osc1VolumeKnob;
	FilmStripKnob *osc2VolumeKnob;
	FilmStripKnob *osc3VolumeKnob;
	FilmStripKnob *portamentoKnob;

	TalComboBox *osc1WaveformTalComboBox;
	TalComboBox *osc2WaveformTalComboBox;

	FilmStripKnob *oscMasterTuneKnob;
	FilmStripKnob *osc1TuneKnob;
	FilmStripKnob *osc2TuneKnob;
	FilmStripKnob *osc1FineTuneKnob;
	FilmStripKnob *osc2FineTuneKnob;

	ImageSlider *filterAttackKnob;
	ImageSlider *filterDecayKnob;
	ImageSlider *filterSustainKnob;
	ImageSlider *filterReleaseKnob;

	ImageSlider *ampAttackKnob;
	ImageSlider *ampDecayKnob;
	ImageSlider *ampSustainKnob;
	ImageSlider *ampReleaseKnob;

    ImageSlider *velocityVolumeKnob;
    ImageSlider *velocityContourKnob;
    ImageSlider *velocityCutoffKnob;
    ImageSlider *pitchwheelCutoffKnob;
    ImageSlider *pitchwheelPitchKnob;
    ImageSlider *highpassKnob;
    ImageSlider *detuneKnob;
    ImageSlider *vintageNoiseKnob;
	ImageSlider *filterDriveKnob;

	ImageToggleButton *oscSyncButton;
	ImageToggleButton *panicButton;
	ImageToggleButton *chorus1Button;
	ImageToggleButton *chorus2Button;

	TalComboBox *voicesTalComboBox;
	TalComboBox *portamentoModeTalComboBox;

	FilmStripKnob *lfo1WaveformKnob;
	FilmStripKnob *lfo2WaveformKnob;

	FilmStripKnob *lfo1RateKnob;
	FilmStripKnob *lfo2RateKnob;

	FilmStripKnob *lfo1AmountKnob;
	FilmStripKnob *lfo2AmountKnob;

	TalComboBox *lfo1DestinationTalComboBox;
	TalComboBox *lfo2DestinationTalComboBox;

	FilmStripKnob *lfo1PhaseKnob;
	FilmStripKnob *lfo2PhaseKnob;

	FilmStripKnob *osc1PwKnob;
	FilmStripKnob *osc2FmKnob;
	FilmStripKnob *osc1PhaseKnob;
	FilmStripKnob *osc2PhaseKnob;
	FilmStripKnob *transposeKnob;

	FilmStripKnob *reverbWetKnob;
	FilmStripKnob *reverbDecayKnob;
	FilmStripKnob *reverbPreDelayKnob;
	FilmStripKnob *reverbHighCutKnob;
	FilmStripKnob *reverbLowCutKnob;

    FilmStripKnob *delayWetKnob;
    FilmStripKnob *delayTimeKnob;
    ImageToggleButton *delaySyncButton;
    ImageToggleButton *delayFactorLButton;
    ImageToggleButton *delayFactorRButton;
    FilmStripKnob *delayHighShelfKnob;
    FilmStripKnob *delayLowShelfKnob;
    FilmStripKnob *delayFeedbackKnob;

	FilmStripKnob *oscBitcrusherKnob;

	FilmStripKnob *ringmodulationKnob;

	FilmStripKnob *freeAdAttackKnob;
	FilmStripKnob *freeAdDecayKnob;
	FilmStripKnob *freeAdAmountKnob;
	TalComboBox *freeAdDestinationTalComboBox;

	ImageToggleButton *lfo1SyncButton;
	ImageToggleButton *lfo1KeyTriggerButton;
	ImageToggleButton *lfo2SyncButton;
	ImageToggleButton *lfo2KeyTriggerButton;

	TalComboBox *envelopeEditorDest1TalComboBox;
	TalComboBox *envelopeEditorSpeedTalComboBox;
    FilmStripKnob *envelopeEditorAmountKnob;
	ImageToggleButton *envelopeOneShotButton;
    ImageToggleButton *envelopeResetButton;
    ImageToggleButton *envelopeFixTempoButton;

	Label *versionLabel;
    Label *infoText;

    ImageToggleButton *loadButton;
    ImageToggleButton *saveButton;

	AudioUtils audioUtils;

    void updateParametersFromFilter();
	FilmStripKnob* addNormalKnob(Component *component, int x, int y, TalCore* const ownerFilter, const Image knobImage, int numOfFrames, const int parameter);
	ImageToggleButton* addNormalButton(Component *component, int x, int y, TalCore* const ownerFilter, const Image buttonImage, bool isKickButton, int parameter);
	ImageSlider* addSlider(Component *component, int x, int y, TalCore* const ownerFilter, const Image sliderImage, int height, int parameter);
	TalComboBox* addTalComboBox(Component *component, int x, int y, int width, TalCore* const ownerFilter, int parameter);

    void updateInfo(Slider* caller);

    // handy wrapper method to avoid having to cast the filter to a DemoJuceFilter
    // every time we need it..

    TalCore* getProcessor() const
    {
        return static_cast <TalCore*> (getAudioProcessor());
    }
};


#endif
