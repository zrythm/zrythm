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

#ifndef __JUCETICE_XEQTABPANEL_HEADER__
#define __JUCETICE_XEQTABPANEL_HEADER__

#include "../XEQPlugin.h"
#include "XEQMain.h"
#include "XEQAbout.h"

//==============================================================================
/**
    This is the Component that our filter will use as its UI.

*/
class XEQTabPanel  : public TabbedComponent
{
public:

    //==============================================================================
    /** Constructor.
    */
    XEQTabPanel (XEQPlugin* plugin_);

    /** Destructor. */
    ~XEQTabPanel () override;

    //==============================================================================
    /**     */
    void currentTabChanged (const int newCurrentTabIndex, const String &newCurrentTabName) override;

    //==============================================================================
    void updateControls ();
    void updateScope ();

private:

    XEQPlugin* plugin;

    XEQMain* mainComponent;
    XEQAbout* aboutComponent;
};


#endif // __JUCETICE_XSYNTHTABPANEL_HEADER__
