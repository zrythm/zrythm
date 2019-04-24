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

#include "XEQTabPanel.h"


//==============================================================================
XEQTabPanel::XEQTabPanel (XEQPlugin* plugin_)
    : TabbedComponent (TabbedButtonBar::TabsAtRight),
      plugin (plugin_)
{
    // sub components
    mainComponent = new XEQMain (plugin);
    aboutComponent = new XEQAbout ();

    addTab ("EQualizer", Colour (0xff575f7d), mainComponent, false);
    addTab ("About", Colour (0xff575f7d), aboutComponent, false);
    setCurrentTabIndex (0);
}

XEQTabPanel::~XEQTabPanel()
{
    clearTabs ();

    deleteAndZero (mainComponent);
    deleteAndZero (aboutComponent);
}

//==============================================================================
void XEQTabPanel::currentTabChanged (const int newCurrentTabIndex,
                                     const String &newCurrentTabName)
{
    switch (newCurrentTabIndex)
    {
    case 0: // mainComponent
        mainComponent->updateControls ();
        break;
/*
    case 1: // mixerComponent
        mixerComponent->updateControls ();
        break;
*/
    }
}

//==============================================================================
void XEQTabPanel::updateControls ()
{
    mainComponent->updateControls ();
}

void XEQTabPanel::updateScope ()
{
    mainComponent->updateScope ();
}

