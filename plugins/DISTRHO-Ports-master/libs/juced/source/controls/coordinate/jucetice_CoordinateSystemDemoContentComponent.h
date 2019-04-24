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

#ifndef __JUCETICE_COORDINATESYSREMDEMOCONTENTCOMPONENT_HEADER__
#define __JUCETICE_COORDINATESYSREMDEMOCONTENTCOMPONENT_HEADER__

#include "jucetice_CoordinateSystem.h"

/*
class CoordinateSystemDemoContentComponent : public Component,
                                             public ChangeBroadcaster,
                                             public ButtonListener,
                                             public ComboBoxListener,
                                             public LabelListener
{  

public:

 CoordinateSystemDemoContentComponent(const String &newEditorName);  
 ///< Constructor.

 virtual ~CoordinateSystemDemoContentComponent();                             
 ///< Destructor.

 virtual void buttonClicked(Button *buttonThatWasClicked);
 ///< Implements the purely virtual buttonClicked()-method of the 
 ///      ButtonListener base-class.

 virtual void comboBoxChanged(ComboBox *comboBoxThatHasChanged);
 ///< Implements the purely virtual comboBoxChanged()-method of the 
 ///     ComboBoxListener base-class.

 virtual void labelTextChanged(Label *labelThatHasChanged); 
 ///< Implements the purely virtual labelTextChanged()-method of the 
 ///     LablelListener base-class.
 
 virtual void setBounds(int x, int y, int width, int height);
 ///< Overrides the setBounds()-method of the Component base-class in order
 ///     to arrange the widgets according to the size.

protected:

 CoordinateSystem* theCoordinateSystem;

 Label*            xAxisSetupLabel;
 Label*            xAnnotationLabel;
 Label*            xAnnotationEditLabel;
 Label*            minXLabel;
 Label*            minXEditLabel;
 Label*            maxXLabel;
 Label*            maxXEditLabel;
 ToggleButton*     logScaleXButton;
 ComboBox*         xAxisComboBox;

 Label*            yAxisSetupLabel;
 Label*            yAnnotationLabel;
 Label*            yAnnotationEditLabel;
 Label*            minYLabel;
 Label*            minYEditLabel;
 Label*            maxYLabel;
 Label*            maxYEditLabel;
 ToggleButton*     logScaleYButton;
 ComboBox*         yAxisComboBox;

 Label*            gridSetupLabel;

 Label*            horizontalGridSetupLabel;
 ToggleButton*     horizontalCoarseGridButton;
 Label*            horizontalCoarseGridIntervalLabel;
 ToggleButton*     horizontalFineGridButton;
 Label*            horizontalFineGridIntervalLabel;

 Label*            verticalGridSetupLabel;
 ToggleButton*     verticalCoarseGridButton;
 Label*            verticalCoarseGridIntervalLabel;
 ToggleButton*     verticalFineGridButton;
 Label*            verticalFineGridIntervalLabel;

 Label*            radialGridSetupLabel;
 ToggleButton*     radialCoarseGridButton;
 Label*            radialCoarseGridIntervalLabel;
 ToggleButton*     radialFineGridButton;
 Label*            radialFineGridIntervalLabel;

 Label*            angularGridSetupLabel;
 ToggleButton*     angularCoarseGridButton;
 Label*            angularCoarseGridIntervalLabel;
 ToggleButton*     angularFineGridButton;
 Label*            angularFineGridIntervalLabel;
};
*/

#endif
