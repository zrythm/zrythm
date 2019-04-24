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
#include "EnvelopeEditor/EnvelopeEditorView.h"
#include "Components/FilmStripKnob.h"

class TalComponent   : public AudioProcessorEditor,
                          public ChangeListener,
                          public SliderListener,
						  public ComboBoxListener
{
public:
    TalComponent(TalCore* const ownerFilter);
    ~TalComponent() override;

    ComboBox* addComboBox(int x, int y, int width, TalCore* const ownerFilter, int parameter);
    FilmStripKnob* addNormalKnob(int x, int y, TalCore* const ownerFilter, const Image knobImage, int numOfFrames, const int parameter);

    void sliderDragStarted (Slider* slider) override;
    void sliderDragEnded (Slider* slider) override;

    void changeListenerCallback (ChangeBroadcaster* source) override;

    void sliderValueChanged (Slider*) override;
    void comboBoxChanged (ComboBox*) override;

    void paint (Graphics& g) override;

    static const char* bmp00128_png;
    static const int bmp00128_pngSize;

    static const char* bmp00135_png;
    static const int bmp00135_pngSize;

private:
    Image* internalCachedBackgroundImage;
    EnvelopeEditorView* envelopeEditorView;

    ComboBox* speedFactorComboBox;
    ComboBox* filtertypeComboBox;

    FilmStripKnob* resonanceKnob;
    FilmStripKnob* depthKnob;
    FilmStripKnob* volumeInKnob;
    FilmStripKnob* volumeOutKnob;

    Label *versionLabel;

    void updateParametersFromFilter();

    TalCore* getFilter() const throw()       { return (TalCore*) getAudioProcessor(); }
};
#endif
