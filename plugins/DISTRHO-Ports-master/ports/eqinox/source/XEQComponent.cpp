/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2004 by Lucio Asnaghi.

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

#include "XEQComponent.h"

#if !JucePlugin_Build_Standalone
AudioDeviceManager deviceManager;
#endif

//==============================================================================
XEQComponent::XEQComponent (XEQPlugin* const ownerFilter_)
    : AudioProcessorEditor (ownerFilter_),
      tabComponent (0)
{
    DBG ("XEQComponent::XEQComponent");
    
    static JuceticeLookAndFeel juceticeLookAndFeel;
    LookAndFeel::setDefaultLookAndFeel (&juceticeLookAndFeel);

    // register ourselves with the plugin - it will use its ChangeBroadcaster base
    // class to tell us when something has changed.
    getFilter()->addChangeListener (this);
//    getFilter()->addListenerToParameters (this);

    // add the main component
    addAndMakeVisible (tabComponent = new XEQTabPanel(getFilter()));
    tabComponent->setTabBarDepth (26);
    tabComponent->setCurrentTabIndex (0);

    // set its size
    setSize (520 + 26, 227);
}

XEQComponent::~XEQComponent()
{
    DBG ("XEQComponent::~XEQComponent");

    deleteAndZero (tabComponent);

    getFilter()->removeChangeListener (this);
//    getFilter()->removeListenerToParameters (this);
}

//==============================================================================
void XEQComponent::resized()
{
    DBG ("XEQComponent::resized");
    
    if (tabComponent)
        tabComponent->setBounds (0, 0, getWidth(), getHeight());
}

//==============================================================================
void XEQComponent::paint (Graphics& g)
{
    g.fillAll (Colour (64, 64, 64));
}

//==============================================================================
void XEQComponent::changeListenerCallback (ChangeBroadcaster* source)
{
    DBG ("XEQComponent::changeListenerCallback");

    if (source == getFilter())
    {
        // if it's the param manager telling us that it's changed
        // we should update the correct control controls
        if (tabComponent)
            tabComponent->updateControls ();
    }
}

//==============================================================================
void XEQComponent::parameterChanged (AudioParameter* parameter, const int index)
{
//    DBG (T("PARAMETER ") + String (index) + T(" changed"));
}
