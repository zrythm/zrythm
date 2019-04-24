/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2009 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2007 by Julian Storer.

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

 ==============================================================================

  Original Code by: braindoc

 ==============================================================================
*/

BEGIN_JUCE_NAMESPACE

#include "jucetice_CoordinateSystemDemoContentComponent.h"

//=============================================================================
/** 
    This is the top-level window that we'll pop up. Inside it, we'll create and
    show a CoordinateSystemDemoContentComponent component.
*/
/*
class CoordinateSystemDemoWindow : public DocumentWindow
{
public:

    CoordinateSystemDemoWindow() : DocumentWindow(T("CoordinateSystem Demo"),
                                                  Colours::lightgrey, 
                                                  DocumentWindow::allButtons, 
                                                  true)
    {
        coordinateSystemDemoContentComponent = 
            new CoordinateSystemDemoContentComponent(String(T("CoordinateSystem Demo")));
        setContentComponent(coordinateSystemDemoContentComponent);

        setResizable(true, true);
        setResizeLimits(640, 480, 40000, 30000);
        setVisible(true);

        centreWithSize(640, 480);

        setTitleBarHeight(24);
   }

   ~CoordinateSystemDemoWindow()
   {
        // the content component will be deleted automatically, so no need to do
        // it here
   }

   void closeButtonPressed()
   {
        // When the user presses the close button, we'll tell the app to quit. This 
        // window will be deleted by the app object as it closes down.
        JUCEApplication::quit();
   }

   void resized()
   {
        DocumentWindow::resized();
        coordinateSystemDemoContentComponent->setBounds(
               0, getTitleBarHeight(), getWidth(), getHeight()-getTitleBarHeight());

        //contentComponent->setSize(getWidth(), getHeight());
   }

   void setBounds(int x, int y, int width, int height)
   {
        DocumentWindow::setBounds(x, y, width, height);
        coordinateSystemDemoContentComponent->setBounds(
               0, getTitleBarHeight(), getWidth(), getHeight()-getTitleBarHeight());

        //contentComponent->setSize(getWidth(), getHeight());
   }

protected:

    CoordinateSystemDemoContentComponent* coordinateSystemDemoContentComponent;
};

*/

END_JUCE_NAMESPACE
