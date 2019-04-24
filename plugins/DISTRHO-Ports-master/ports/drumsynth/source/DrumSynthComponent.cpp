/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2008 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2004 by Julian Storer.

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

 ------------------------------------------------------------------------------

 If you'd like to release a closed-source product which uses JUCE, commercial
 licenses are also available: visit www.rawmaterialsoftware.com/juce for
 more information.

 ==============================================================================
*/

#include "DrumSynthComponent.h"


//==============================================================================
DrumSynthComponent::DrumSynthComponent (DrumSynthPlugin* const ownerFilter_)
    : AudioProcessorEditor (ownerFilter_)
{
    setLookAndFeel (&juceticeLookAndFeel);
    LookAndFeel::setDefaultLookAndFeel (&juceticeLookAndFeel);

    // register ourselves with the plugin - it will use its ChangeBroadcaster base
    // class to tell us when something has changed.
    getFilter()->addChangeListener (this);
//    getFilter()->addListenerToParameters (this);

    // add the main component
    addAndMakeVisible (mainComponent = new DrumSynthMain (getFilter(), this));

    setSize (mainComponent->getWidth(),
             mainComponent->getHeight());

    mainComponent->updateControls ();
}

DrumSynthComponent::~DrumSynthComponent()
{
    deleteAndZero (mainComponent);

    getFilter()->removeChangeListener (this);
//    getFilter()->removeListenerToParameters (this);
}

//==============================================================================
void DrumSynthComponent::resized()
{
    // mainComponent->setBounds (0, 0, getWidth(), getHeight());
}

//==============================================================================
void DrumSynthComponent::paint (Graphics& g)
{
}

//==============================================================================
void DrumSynthComponent::changeListenerCallback (ChangeBroadcaster* source)
{
    if (source == getFilter())
    {
        mainComponent->updateControls ();
    }
}

//==============================================================================
void DrumSynthComponent::parameterChanged (AudioParameter* parameter, const int index)
{
}
