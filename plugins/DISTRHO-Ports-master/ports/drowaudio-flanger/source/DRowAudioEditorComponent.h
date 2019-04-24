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

#ifndef DROWAUDIOEDITORCOMPONENT_H
#define DROWAUDIOEDITORCOMPONENT_H

#include "includes.h"
#include "DRowAudioFilter.h"
#include "Parameters.h"
#include "Resources/flanger_title.h"

#include "dRowAudio_PluginLookAndFeel.h"


//==============================================================================
/**
    This is the Component that our filter will use as its UI.

    One or more of these is created by the DRowAudioFilter::createEditor() method,
    and they will be deleted at some later time by the wrapper code.

    To demonstrate the correct way of connecting a filter to its UI, this
    class is a ChangeListener, and our demo filter is a ChangeBroadcaster. The
    editor component registers with the filter when it's created and deregisters
    when it's destroyed. When the filter's parameters are changed, it broadcasts
    a message and this editor responds by updating its display.
*/
class DRowAudioEditorComponent   : public AudioProcessorEditor,
								   public ChangeListener,
								   public SliderListener
{
public:
    /** Constructor.

        When created, this will register itself with the filter for changes. It's
        safe to assume that the filter won't be deleted before this object is.
    */
    DRowAudioEditorComponent (DRowAudioFilter* const ownerFilter);

    /** Destructor. */
    ~DRowAudioEditorComponent() override;

    //==============================================================================
    /** Our demo filter is a ChangeBroadcaster, and will call us back when one of
        its parameters changes.
    */
    void changeListenerCallback (ChangeBroadcaster* source) override;

    void sliderValueChanged (Slider*) override;
    void sliderDragStarted(Slider*) override;
    void sliderDragEnded(Slider*) override;

    //==============================================================================
    /** Standard Juce paint callback. */
    void paint (Graphics& g) override;

    /** Standard Juce resize callback. */
    void resized() override;

private:
    //==============================================================================
    ScopedPointer<dRowLookAndFeel> customLookAndFeel;

    OwnedArray <Slider> sliders;
    OwnedArray <Label> sliderLabels;

    void updateParametersFromFilter();

    // handy wrapper method to avoid having to cast the filter to a DRowAudioFilter
    // every time we need it..
    DRowAudioFilter* getFilter() const noexcept       { return (DRowAudioFilter*) getAudioProcessor(); }
};


#endif