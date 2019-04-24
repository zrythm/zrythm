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

/*

//----------------------------------------------------------------------------
// construction/destruction:

CoordinateSystemDemoContentComponent::CoordinateSystemDemoContentComponent(
 const String &newEditorName) : Component(newEditorName)
{
 setSize(640, 480-24);

 theCoordinateSystem = new CoordinateSystem();
 theCoordinateSystem->setRange(-4.5, +4.5, -4.5, +4.5);
 addAndMakeVisible(theCoordinateSystem);

 //----------------------------------------------------------------------------
 // x-axis setup widgets:

 xAxisSetupLabel = new Label( String(T("xAxisSetupLabel")), String(T("x-axis setup:")) );
 xAxisSetupLabel->setJustificationType(Justification::centred);
 addAndMakeVisible( xAxisSetupLabel );

 xAnnotationLabel = new Label( String(T("xAnnotationLabel")), String(T("annotation:")) );
 xAnnotationLabel->setJustificationType(Justification::centredLeft);
 addAndMakeVisible( xAnnotationLabel );

 xAnnotationEditLabel = new Label( String(T("xAnnotationEditLabel")), String(T("x")) );
 xAnnotationEditLabel->setJustificationType(Justification::centredLeft);
 xAnnotationEditLabel->setColour(Label::outlineColourId, Colours::black);
 xAnnotationEditLabel->setEditable(true, true);
 xAnnotationEditLabel->addListener(this);
 addAndMakeVisible( xAnnotationEditLabel );

 minXLabel = new Label( String(T("MinXLabel")), String(T("min:")) );
 minXLabel->setJustificationType(Justification::centredLeft);
 addAndMakeVisible( minXLabel );

 minXEditLabel = new Label( String(T("MinXEditLabel")), String(T("-4.5")) );
 minXEditLabel->setJustificationType(Justification::centredLeft);
 minXEditLabel->setColour(Label::outlineColourId, Colours::black);
 minXEditLabel->setEditable(true, true);
 minXEditLabel->addListener(this);
 addAndMakeVisible( minXEditLabel );

 maxXLabel = new Label( String(T("MaxXLabel")), String(T("max:")) );
 maxXLabel->setJustificationType(Justification::centredLeft);
 addAndMakeVisible( maxXLabel );

 maxXEditLabel = new Label( String(T("MaxXEditLabel")), String(T("4.5")) );
 maxXEditLabel->setJustificationType(Justification::centredLeft);
 maxXEditLabel->setColour(Label::outlineColourId, Colours::black);
 maxXEditLabel->setEditable(true, true);
 maxXEditLabel->addListener(this);
 addAndMakeVisible( maxXEditLabel );

 logScaleXButton = new ToggleButton(String("log"));
 logScaleXButton->setClickingTogglesState(true);
 logScaleXButton->setToggleState(false, true);
 logScaleXButton->addButtonListener(this);
 addAndMakeVisible(logScaleXButton);

 xAxisComboBox = new ComboBox(String(T("xAxisComboBox")));
 xAxisComboBox->addItem(String(T("no axis")),     1);
 xAxisComboBox->addItem(String(T("at zero")),     2);
 xAxisComboBox->addItem(String(T("at top")),      3);
 xAxisComboBox->addItem(String(T("at bottom")),   4);
 xAxisComboBox->setSelectedId(2);
 xAxisComboBox->addListener(this);
 addAndMakeVisible( xAxisComboBox );

 //----------------------------------------------------------------------------
 // y-axis setup widgets:

 yAxisSetupLabel = new Label( String(T("yAxisSetupLabel")), String(T("y-axis setup:")) );
 yAxisSetupLabel->setJustificationType(Justification::centred);
 addAndMakeVisible( yAxisSetupLabel );

 yAnnotationLabel = new Label( String(T("yAnnotationLabel")), String(T("annotation:")) );
 yAnnotationLabel->setJustificationType(Justification::centredLeft);
 addAndMakeVisible( yAnnotationLabel );

 yAnnotationEditLabel = new Label( String(T("yAnnotationEditLabel")), String(T("y")) );
 yAnnotationEditLabel->setJustificationType(Justification::centredLeft);
 yAnnotationEditLabel->setColour(Label::outlineColourId, Colours::black);
 yAnnotationEditLabel->setEditable(true, true);
 yAnnotationEditLabel->addListener(this);
 addAndMakeVisible( yAnnotationEditLabel );

 minYLabel = new Label( String(T("MinYLabel")), String(T("min:")) );
 minYLabel->setJustificationType(Justification::centredLeft);
 addAndMakeVisible( minYLabel );

 minYEditLabel = new Label( String(T("MinYEditLabel")), String(T("-4.5")) );
 minYEditLabel->setJustificationType(Justification::centredLeft);
 minYEditLabel->setColour(Label::outlineColourId, Colours::black);
 minYEditLabel->setEditable(true, true);
 minYEditLabel->addListener(this);
 addAndMakeVisible( minYEditLabel );

 maxYLabel = new Label( String(T("MaxYLabel")), String(T("max:")) );
 maxYLabel->setJustificationType(Justification::centredLeft);
 addAndMakeVisible( maxYLabel );

 maxYEditLabel = new Label( String(T("MaxYEditLabel")), String(T("4.5")) );
 maxYEditLabel->setJustificationType(Justification::centredLeft);
 maxYEditLabel->setColour(Label::outlineColourId, Colours::black);
 maxYEditLabel->setEditable(true, true);
 maxYEditLabel->addListener(this);
 addAndMakeVisible( maxYEditLabel );

 logScaleYButton = new ToggleButton(String("log"));
 logScaleYButton->setClickingTogglesState(true);
 logScaleYButton->setToggleState(false, true);
 logScaleYButton->addButtonListener(this);
 addAndMakeVisible(logScaleYButton);

 yAxisComboBox = new ComboBox(String(T("yAxisComboBox")));
 yAxisComboBox->addItem(String(T("no axis")),     1);
 yAxisComboBox->addItem(String(T("at zero")),     2);
 yAxisComboBox->addItem(String(T("at left")),     3);
 yAxisComboBox->addItem(String(T("at right")),    4);
 yAxisComboBox->setSelectedId(2);
 yAxisComboBox->addListener(this);
 addAndMakeVisible( yAxisComboBox );

 //----------------------------------------------------------------------------
 // grid setup widgets:

 gridSetupLabel = new Label(String(T("gridSetupLabel")), 
                            String(T("grid setup:")) );
 gridSetupLabel->setJustificationType(Justification::centred);
 addAndMakeVisible( gridSetupLabel );

 //----------------------------------------------------------------------------
 // horizontal grid setup widgets:

 horizontalGridSetupLabel = new Label(String(T("horizontalGridSetupLabel")), 
                                      String(T("horizontal:")) );
 horizontalGridSetupLabel->setJustificationType(Justification::centredLeft);
 addAndMakeVisible( horizontalGridSetupLabel );

 horizontalCoarseGridButton = new ToggleButton(String("coarse:"));
 horizontalCoarseGridButton->setClickingTogglesState(true);
 horizontalCoarseGridButton->setToggleState(false, true);
 horizontalCoarseGridButton->addButtonListener(this);
 addAndMakeVisible(horizontalCoarseGridButton);

 horizontalCoarseGridIntervalLabel = 
  new Label( String(T("horizontalCoarseGridIntervalLabel")), String(T("1.0")) );
 horizontalCoarseGridIntervalLabel->setJustificationType(Justification::centredLeft);
 horizontalCoarseGridIntervalLabel->setColour(Label::outlineColourId, Colours::black);
 horizontalCoarseGridIntervalLabel->setEditable(true, true);
 horizontalCoarseGridIntervalLabel->addListener(this);
 addAndMakeVisible( horizontalCoarseGridIntervalLabel );

 horizontalFineGridButton = new ToggleButton(String("fine:"));
 horizontalFineGridButton->setClickingTogglesState(true);
 horizontalFineGridButton->setToggleState(false, true);
 horizontalFineGridButton->addButtonListener(this);
 addAndMakeVisible(horizontalFineGridButton);

 horizontalFineGridIntervalLabel = 
  new Label( String(T("horizontalFineGridIntervalLabel")), String(T("0.1")) );
 horizontalFineGridIntervalLabel->setJustificationType(Justification::centredLeft);
 horizontalFineGridIntervalLabel->setColour(Label::outlineColourId, Colours::black);
 horizontalFineGridIntervalLabel->setEditable(true, true);
 horizontalFineGridIntervalLabel->addListener(this);
 addAndMakeVisible( horizontalFineGridIntervalLabel );

 //----------------------------------------------------------------------------
 // vertical grid setup widgets:

 verticalGridSetupLabel = new Label(String(T("verticalGridSetupLabel")), 
                                      String(T("vertical:")) );
 verticalGridSetupLabel->setJustificationType(Justification::centredLeft);
 addAndMakeVisible( verticalGridSetupLabel );

 verticalCoarseGridButton = new ToggleButton(String("coarse:"));
 verticalCoarseGridButton->setClickingTogglesState(true);
 verticalCoarseGridButton->setToggleState(false, true);
 verticalCoarseGridButton->addButtonListener(this);
 addAndMakeVisible(verticalCoarseGridButton);

 verticalCoarseGridIntervalLabel = 
  new Label( String(T("verticalCoarseGridIntervalLabel")), String(T("1.0")) );
 verticalCoarseGridIntervalLabel->setJustificationType(Justification::centredLeft);
 verticalCoarseGridIntervalLabel->setColour(Label::outlineColourId, Colours::black);
 verticalCoarseGridIntervalLabel->setEditable(true, true);
 verticalCoarseGridIntervalLabel->addListener(this);
 addAndMakeVisible( verticalCoarseGridIntervalLabel );

 verticalFineGridButton = new ToggleButton(String("fine:"));
 verticalFineGridButton->setClickingTogglesState(true);
 verticalFineGridButton->setToggleState(false, true);
 verticalFineGridButton->addButtonListener(this);
 addAndMakeVisible(verticalFineGridButton);

 verticalFineGridIntervalLabel = 
  new Label( String(T("verticalFineGridIntervalLabel")), String(T("0.1")) );
 verticalFineGridIntervalLabel->setJustificationType(Justification::centredLeft);
 verticalFineGridIntervalLabel->setColour(Label::outlineColourId, Colours::black);
 verticalFineGridIntervalLabel->setEditable(true, true);
 verticalFineGridIntervalLabel->addListener(this);
 addAndMakeVisible( verticalFineGridIntervalLabel );

 //----------------------------------------------------------------------------
 // radial grid setup widgets:

 radialGridSetupLabel = new Label(String(T("radialGridSetupLabel")), 
                                      String(T("radial:")) );
 radialGridSetupLabel->setJustificationType(Justification::centredLeft);
 addAndMakeVisible( radialGridSetupLabel );

 radialCoarseGridButton = new ToggleButton(String("coarse:"));
 radialCoarseGridButton->setClickingTogglesState(true);
 radialCoarseGridButton->setToggleState(false, true);
 radialCoarseGridButton->addButtonListener(this);
 addAndMakeVisible(radialCoarseGridButton);

 radialCoarseGridIntervalLabel = 
  new Label( String(T("radialCoarseGridIntervalLabel")), String(T("1.0")) );
 radialCoarseGridIntervalLabel->setJustificationType(Justification::centredLeft);
 radialCoarseGridIntervalLabel->setColour(Label::outlineColourId, Colours::black);
 radialCoarseGridIntervalLabel->setEditable(true, true);
 radialCoarseGridIntervalLabel->addListener(this);
 addAndMakeVisible( radialCoarseGridIntervalLabel );

 radialFineGridButton = new ToggleButton(String("fine:"));
 radialFineGridButton->setClickingTogglesState(true);
 radialFineGridButton->setToggleState(false, true);
 radialFineGridButton->addButtonListener(this);
 addAndMakeVisible(radialFineGridButton);

 radialFineGridIntervalLabel = 
  new Label( String(T("radialFineGridIntervalLabel")), String(T("0.1")) );
 radialFineGridIntervalLabel->setJustificationType(Justification::centredLeft);
 radialFineGridIntervalLabel->setColour(Label::outlineColourId, Colours::black);
 radialFineGridIntervalLabel->setEditable(true, true);
 radialFineGridIntervalLabel->addListener(this);
 addAndMakeVisible( radialFineGridIntervalLabel );

 //----------------------------------------------------------------------------
 // angular grid setup widgets:

 angularGridSetupLabel = new Label(String(T("angularGridSetupLabel")), 
                                      String(T("angular:")) );
 angularGridSetupLabel->setJustificationType(Justification::centredLeft);
 addAndMakeVisible( angularGridSetupLabel );

 angularCoarseGridButton = new ToggleButton(String("coarse:"));
 angularCoarseGridButton->setClickingTogglesState(true);
 angularCoarseGridButton->setToggleState(false, true);
 angularCoarseGridButton->addButtonListener(this);
 addAndMakeVisible(angularCoarseGridButton);

 angularCoarseGridIntervalLabel = 
  new Label( String(T("angularCoarseGridIntervalLabel")), String(T("15")) );
 angularCoarseGridIntervalLabel->setJustificationType(Justification::centredLeft);
 angularCoarseGridIntervalLabel->setColour(Label::outlineColourId, Colours::black);
 angularCoarseGridIntervalLabel->setEditable(true, true);
 angularCoarseGridIntervalLabel->addListener(this);
 addAndMakeVisible( angularCoarseGridIntervalLabel );

 angularFineGridButton = new ToggleButton(String("fine:"));
 angularFineGridButton->setClickingTogglesState(true);
 angularFineGridButton->setToggleState(false, true);
 angularFineGridButton->addButtonListener(this);
 addAndMakeVisible(angularFineGridButton);

 angularFineGridIntervalLabel = 
  new Label( String(T("angularFineGridIntervalLabel")), String(T("5")) );
 angularFineGridIntervalLabel->setJustificationType(Justification::centredLeft);
 angularFineGridIntervalLabel->setColour(Label::outlineColourId, Colours::black);
 angularFineGridIntervalLabel->setEditable(true, true);
 angularFineGridIntervalLabel->addListener(this);
 addAndMakeVisible( angularFineGridIntervalLabel );

}

CoordinateSystemDemoContentComponent::~CoordinateSystemDemoContentComponent()
{
 deleteAllChildren();
}

//----------------------------------------------------------------------------
// callbacks:

void CoordinateSystemDemoContentComponent::buttonClicked(Button *buttonThatWasClicked)
{
 if( buttonThatWasClicked == logScaleXButton )
  theCoordinateSystem->useLogarithmicScaleX(logScaleXButton->getToggleState());
 else if( buttonThatWasClicked == logScaleYButton )
  theCoordinateSystem->useLogarithmicScaleY(logScaleYButton->getToggleState());
 else if( buttonThatWasClicked == horizontalCoarseGridButton )
  theCoordinateSystem->setHorizontalCoarseGrid
  (
   horizontalCoarseGridIntervalLabel->getText().getDoubleValue(),
   horizontalCoarseGridButton->getToggleState()
  );
 else if( buttonThatWasClicked == horizontalFineGridButton )
  theCoordinateSystem->setHorizontalFineGrid
  (
   horizontalFineGridIntervalLabel->getText().getDoubleValue(),
   horizontalFineGridButton->getToggleState()
  );
 else if( buttonThatWasClicked == verticalCoarseGridButton )
  theCoordinateSystem->setVerticalCoarseGrid
  (
   verticalCoarseGridIntervalLabel->getText().getDoubleValue(),
   verticalCoarseGridButton->getToggleState()
  );
 else if( buttonThatWasClicked == verticalFineGridButton )
  theCoordinateSystem->setVerticalFineGrid
  (
   verticalFineGridIntervalLabel->getText().getDoubleValue(),
   verticalFineGridButton->getToggleState()
  );
 else if( buttonThatWasClicked == radialCoarseGridButton )
  theCoordinateSystem->setRadialCoarseGrid
  (
   radialCoarseGridIntervalLabel->getText().getDoubleValue(),
   radialCoarseGridButton->getToggleState()
  );
 else if( buttonThatWasClicked == radialFineGridButton )
  theCoordinateSystem->setRadialFineGrid
  (
   radialFineGridIntervalLabel->getText().getDoubleValue(),
   radialFineGridButton->getToggleState()
  );
 else if( buttonThatWasClicked == angularCoarseGridButton )
  theCoordinateSystem->setAngularCoarseGrid
  (
   angularCoarseGridIntervalLabel->getText().getDoubleValue(),
   angularCoarseGridButton->getToggleState()
  );
 else if( buttonThatWasClicked == angularFineGridButton )
  theCoordinateSystem->setAngularFineGrid
  (
   angularFineGridIntervalLabel->getText().getDoubleValue(),
   angularFineGridButton->getToggleState()
  );
}

void CoordinateSystemDemoContentComponent::comboBoxChanged(ComboBox *comboBoxThatHasChanged)
{
 if( comboBoxThatHasChanged == xAxisComboBox )
 {
  switch( xAxisComboBox->getSelectedId() )
  {
  case 1:
   theCoordinateSystem->setAxisPositionX(CoordinateSystem::INVISIBLE);
   break;
  case 2:
   theCoordinateSystem->setAxisPositionX(CoordinateSystem::ZERO);
   break;
  case 3:
   theCoordinateSystem->setAxisPositionX(CoordinateSystem::TOP);
   break;
  case 4:
   theCoordinateSystem->setAxisPositionX(CoordinateSystem::BOTTOM);
   break;
  }
 }
 else if( comboBoxThatHasChanged == yAxisComboBox )
 {
  switch( yAxisComboBox->getSelectedId() )
  {
  case 1:
   theCoordinateSystem->setAxisPositionY(CoordinateSystem::INVISIBLE);
   break;
  case 2:
   theCoordinateSystem->setAxisPositionY(CoordinateSystem::ZERO);
   break;
  case 3:
   theCoordinateSystem->setAxisPositionY(CoordinateSystem::LEFT);
   break;
  case 4:
   theCoordinateSystem->setAxisPositionY(CoordinateSystem::RIGHT);
   break;
  }
 }
}

void CoordinateSystemDemoContentComponent::labelTextChanged(Label *labelThatHasChanged)
{
 if( labelThatHasChanged == xAnnotationEditLabel )
  theCoordinateSystem->setAxisLabelX(xAnnotationEditLabel->getText());
 else if( labelThatHasChanged == minXEditLabel )
  theCoordinateSystem->setMinX(minXEditLabel->getText().getDoubleValue());
 else if( labelThatHasChanged == maxXEditLabel )
  theCoordinateSystem->setMaxX(maxXEditLabel->getText().getDoubleValue());
 else if( labelThatHasChanged == yAnnotationEditLabel )
  theCoordinateSystem->setAxisLabelY(yAnnotationEditLabel->getText());
 else if( labelThatHasChanged == minYEditLabel )
  theCoordinateSystem->setMinY(minYEditLabel->getText().getDoubleValue());
 else if( labelThatHasChanged == maxYEditLabel )
  theCoordinateSystem->setMaxY(maxYEditLabel->getText().getDoubleValue());
 else if( labelThatHasChanged == horizontalCoarseGridIntervalLabel )
  theCoordinateSystem->setHorizontalCoarseGrid
  (
   horizontalCoarseGridIntervalLabel->getText().getDoubleValue(),
   horizontalCoarseGridButton->getToggleState()
  );
 else if( labelThatHasChanged == horizontalFineGridIntervalLabel )
  theCoordinateSystem->setHorizontalFineGrid
  (
   horizontalFineGridIntervalLabel->getText().getDoubleValue(),
   horizontalFineGridButton->getToggleState()
  );
 else if( labelThatHasChanged == verticalCoarseGridIntervalLabel )
  theCoordinateSystem->setVerticalCoarseGrid
  (
   verticalCoarseGridIntervalLabel->getText().getDoubleValue(),
   verticalCoarseGridButton->getToggleState()
  );
 else if( labelThatHasChanged == verticalFineGridIntervalLabel )
  theCoordinateSystem->setVerticalFineGrid
  (
   verticalFineGridIntervalLabel->getText().getDoubleValue(),
   verticalFineGridButton->getToggleState()
  );
 else if( labelThatHasChanged == radialCoarseGridIntervalLabel )
  theCoordinateSystem->setRadialCoarseGrid
  (
   radialCoarseGridIntervalLabel->getText().getDoubleValue(),
   radialCoarseGridButton->getToggleState()
  );
 else if( labelThatHasChanged == radialFineGridIntervalLabel )
  theCoordinateSystem->setRadialFineGrid
  (
   radialFineGridIntervalLabel->getText().getDoubleValue(),
   radialFineGridButton->getToggleState()
  );
 else if( labelThatHasChanged == angularCoarseGridIntervalLabel )
  theCoordinateSystem->setAngularCoarseGrid
  (
   angularCoarseGridIntervalLabel->getText().getDoubleValue(),
   angularCoarseGridButton->getToggleState()
  );
 else if( labelThatHasChanged == angularFineGridIntervalLabel )
  theCoordinateSystem->setAngularFineGrid
  (
   angularFineGridIntervalLabel->getText().getDoubleValue(),
   angularFineGridButton->getToggleState()
  );
}

//----------------------------------------------------------------------------
// appearance stuff:

void CoordinateSystemDemoContentComponent::setBounds(int x, int y, 
                                                     int width, int height)
{
 Component::setBounds(x, y, width, height);

 int xL, xR, yT, yB; // left, right, top and bottom for the child-component
 int w, h;           // width and height for the child component

 xL = 4;
 xR = (3*getWidth()/4)-4;
 w  = xR-xL;
 yT = 4;
 yB = getHeight()-4;
 h  = yB-yT;
 theCoordinateSystem->setBounds(xL, yT, w, h);

 xL = (3*getWidth()/4)+4;
 xR = getWidth()-4;
 w  = xR-xL;
 yT = 4;
 yB = 20;
 h  = yB-yT;
 xAxisSetupLabel->setBounds(xL, yT, w, 16);
 yT += 20;
 xAnnotationLabel->setBounds(xL, yT, 80, 16);
 xAnnotationEditLabel->setBounds(xL+80, yT, w-80, 16);
 yT += 20;
 minXLabel->setBounds(xL, yT, 36, 16);
 minXEditLabel->setBounds(xL+36, yT, w/2-36, 16);
 maxXLabel->setBounds(xL+w/2, yT, 36, h);
 maxXEditLabel->setBounds(xL+w/2+36, yT, w/2-36, 16);
 yT += 24;
 logScaleXButton->setBounds(xL, yT-2, 48, 24);
 xAxisComboBox->setBounds(xL+48, yT, w-48, 20);

 yT += 32;
 yAxisSetupLabel->setBounds(xL, yT, w, 16);
 yT += 20;
 yAnnotationLabel->setBounds(xL, yT, 80, 16);
 yAnnotationEditLabel->setBounds(xL+80, yT, w-80, 16);
 yT += 20;
 minYLabel->setBounds(xL, yT, 36, 16);
 minYEditLabel->setBounds(xL+36, yT, w/2-36, 16);
 maxYLabel->setBounds(xL+w/2, yT, 36, h);
 maxYEditLabel->setBounds(xL+w/2+36, yT, w/2-36, 16);
 yT += 24;
 logScaleYButton->setBounds(xL, yT-2, 48, 24);
 yAxisComboBox->setBounds(xL+48, yT, w-48, 20);

 yT += 32;
 gridSetupLabel->setBounds(xL, yT, w, 16);

 yT += 20;
 horizontalGridSetupLabel->setBounds(xL, yT, w, 16);
 yT += 16;
 horizontalCoarseGridButton->setBounds(xL-4, yT-4, 72, 24);
 horizontalCoarseGridIntervalLabel->setBounds(xL+72, yT, w-72, 16);
 yT += 16;
 horizontalFineGridButton->setBounds(xL-4, yT-4, 72, 24);
 horizontalFineGridIntervalLabel->setBounds(xL+72, yT, w-72, 16);

 yT += 20;
 verticalGridSetupLabel->setBounds(xL, yT, w, 16);
 yT += 16;
 verticalCoarseGridButton->setBounds(xL-4, yT-4, 72, 24);
 verticalCoarseGridIntervalLabel->setBounds(xL+72, yT, w-72, 16);
 yT += 16;
 verticalFineGridButton->setBounds(xL-4, yT-4, 72, 24);
 verticalFineGridIntervalLabel->setBounds(xL+72, yT, w-72, 16);

 yT += 20;
 radialGridSetupLabel->setBounds(xL, yT, w, 16);
 yT += 16;
 radialCoarseGridButton->setBounds(xL-4, yT-4, 72, 24);
 radialCoarseGridIntervalLabel->setBounds(xL+72, yT, w-72, 16);
 yT += 16;
 radialFineGridButton->setBounds(xL-4, yT-4, 72, 24);
 radialFineGridIntervalLabel->setBounds(xL+72, yT, w-72, 16);

 yT += 20;
 angularGridSetupLabel->setBounds(xL, yT, w, 16);
 yT += 16;
 angularCoarseGridButton->setBounds(xL-4, yT-4, 72, 24);
 angularCoarseGridIntervalLabel->setBounds(xL+72, yT, w-72, 16);
 yT += 16;
 angularFineGridButton->setBounds(xL-4, yT-4, 72, 24);
 angularFineGridIntervalLabel->setBounds(xL+72, yT, w-72, 16);
}
*/

END_JUCE_NAMESPACE