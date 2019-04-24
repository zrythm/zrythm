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

CoordinateSystem::CoordinateSystem(const String &name) 
    : Component(name)
{
    scaleX                        =  1.0;
    scaleY                        =  1.0;
    pixelsPerIntervalX            =  100.0;
    pixelsPerIntervalY            =  100.0;

    axisPositionX                 =  ZERO;
    axisPositionY                 =  ZERO;
    labelPositionX                =  ABOVE_AXIS;
    labelPositionY                =  RIGHT_TO_AXIS;
    axisValuesPositionX           =  BELOW_AXIS;
    axisValuesPositionY           =  LEFT_TO_AXIS;

    labelX                        =  String("x");
    labelY                        =  String("y");

    horizontalCoarseGridIsVisible =  false;
    horizontalFineGridIsVisible	  =  false;
    verticalCoarseGridIsVisible   =  false;
    verticalFineGridIsVisible	    =  false;
    radialCoarseGridIsVisible     =  false;
    radialFineGridIsVisible       =  false;
    angularCoarseGridIsVisible    =  false;
    angularFineGridIsVisible      =  false;

    horizontalCoarseGridInterval  =  1.0;
    horizontalFineGridInterval    =  0.1;
    verticalCoarseGridInterval    =  1.0;
    verticalFineGridInterval      =  0.1;
    radialCoarseGridInterval      =  1.0;
    radialFineGridInterval        =  0.1;
    angularCoarseGridInterval     =  15.0;  // 15 degrees
    angularFineGridInterval       =  5.0;   // 5 degrees

    angleIsInDegrees              =  true;

    isLogScaledX	                 =  false;
    logBaseX	                     =  2.0;
    isLogScaledY	                 =  false;
    logBaseY	                     =  2.0;
    isLogScaledRadius             =  false;
    logBaseRadius                 =  2.0;

    // set up the number of decimal digits after th point for the axes 
    // annotation:
    //numDigitsX                    = 1;
    //numDigitsY                    = 1;

    // initialize the function-pointers for value->string conversion
    stringConversionFunctionX = &valueToString1;
    stringConversionFunctionY = &valueToString1;
    //stringConversionFunctionY = &decibelsToStringWithUnit1;

    // set up the default colour-scheme:
    backgroundColour              = Colours::white;
    axesColour                    = Colours::black;
    coarseGridColour              = Colours::grey;
    fineGridColour                = Colours::lightgrey;
    outlineColour                 = Colours::black;

    /*
    // cool colorscheme for radar-alike plots
    backgroundColour              = Colours::black;
    axesColour                    = Colours::limegreen;
    coarseGridColour              = Colours::green;
    fineGridColour                = Colours::darkgreen;
    outlineColour                 = Colours::black;
    */

    // initialize the component-size and the image-size to 1x1 pixels, without
    // such initializations, a JUCE-breakpoint will be triggered or other screws
    // happen:
    backgroundImage = Image();
    setBounds(0, 0, 1, 1);
    backgroundImage = Image(Image::RGB, 1, 1, true);
    updateBackgroundImage();
}

CoordinateSystem::~CoordinateSystem()
{

}

//----------------------------------------------------------------------------
void CoordinateSystem::setBounds(int x, int y, int width, int height)
{
    Component::setBounds(x, y, width, height);

    updateScaleFactors();
    updateBackgroundImage();
}

//----------------------------------------------------------------------------

void CoordinateSystem::setAxisLabels(const String &newLabelX, 
				                                 const String &newLabelY, 
				                                 int newLabelPositionX,
				                                 int newLabelPositionY)
{
    labelX         = newLabelX;
    labelY         = newLabelY;
    labelPositionX = newLabelPositionX;
    labelPositionY = newLabelPositionY;

    updateBackgroundImage();
}

void CoordinateSystem::setAxisLabelX(const String& newLabelX, 
                                     int newLabelPositionX)
{
    labelX         = newLabelX;
    labelPositionX = newLabelPositionX;
    updateBackgroundImage();
}

void CoordinateSystem::setAxisLabelY(const String& newLabelY, 
                                     int newLabelPositionY)
{
    labelY         = newLabelY;
    labelPositionY = newLabelPositionY;
    updateBackgroundImage();
}

void CoordinateSystem::setValuesPositionX(int newValuesPositionX)
{
    if( newValuesPositionX == NO_ANNOTATION ||
        newValuesPositionX == BELOW_AXIS    ||
        newValuesPositionX == ABOVE_AXIS      )
    {
        axisValuesPositionX = newValuesPositionX;
    }
}

void CoordinateSystem::setValuesPositionY(int newValuesPositionY)
{
    if( newValuesPositionY == NO_ANNOTATION ||
        newValuesPositionY == LEFT_TO_AXIS  ||
        newValuesPositionY == RIGHT_TO_AXIS   )
    {
        axisValuesPositionY = newValuesPositionY;
    }
}

void CoordinateSystem::setStringConversionFunctionX(
 String (*newConversionFunctionX) (double valueToBeConverted) )
{
    stringConversionFunctionX = newConversionFunctionX;
}

void CoordinateSystem::setStringConversionFunctionY(
 String (*newConversionFunctionY) (double valueToBeConverted) )
{
    stringConversionFunctionY = newConversionFunctionY;
}

void CoordinateSystem::setRange(double newMinX, double newMaxX, 
			                            	double newMinY, double newMaxY)
{
    range.setMinX(newMinX);
    range.setMaxX(newMaxX);
    range.setMinY(newMinY);
    range.setMaxY(newMaxY);

    updateScaleFactors();
    updateBackgroundImage();
}

void CoordinateSystem::setRange(CoordinateSystemRange newRange)
{
    range = newRange;
    updateScaleFactors();
    updateBackgroundImage();
}

void CoordinateSystem::setMinX(double newMinX)
{
    range.setMinX(newMinX);
    updateScaleFactors();
    updateBackgroundImage();
}

void CoordinateSystem::setMinY(double newMinY)
{
    range.setMinY(newMinY);
    updateScaleFactors();
    updateBackgroundImage();
}

void CoordinateSystem::setMaxX(double newMaxX)
{
 range.setMaxX(newMaxX);
 updateScaleFactors();
 updateBackgroundImage();
}

void CoordinateSystem::setMaxY(double newMaxY)
{
 range.setMaxY(newMaxY);
 updateScaleFactors();
 updateBackgroundImage();
}

CoordinateSystemRange // return value
 CoordinateSystem::getMaximumMeaningfulRange(double relativeMargin)
{
 // require at least 1.0% margin:
 jassert( relativeMargin >= 1.0 );
 if( relativeMargin < 1.0 )
  relativeMargin = 1.0;

 CoordinateSystemRange r;

 double minX   = range.getMinX();
 double maxX   = range.getMaxX();
 double width  = maxX-minX;
 double minY   = range.getMinY();
 double maxY   = range.getMaxY();
 double height = maxY-minY;

 r.setMinX(minX - 0.01*relativeMargin*width);
 r.setMaxX(maxX + 0.01*relativeMargin*width);
 r.setMinY(minY - 0.01*relativeMargin*height);
 r.setMaxY(maxY + 0.01*relativeMargin*height);

 return r;
}

void CoordinateSystem::setHorizontalCoarseGrid(double newGridInterval, 
                                               bool   shouldBeVisible)
{
 // for logarithmic scaling of an axis, we need the grid-intervals to be 
 // strictly greater than unity because the coordinate of a grid-line results
 // from the coordinate of the previous grid-line via multiplication - we 
 // would end up drawing an infinite number of grid-lines at the same 
 // coordinate with a unity-factor and denser and denser grid-lines when 
 // approaching zero with a factor lower than unity.
 if( isLogScaledY )
 {
  jassert(newGridInterval > 1.00001);
  if( newGridInterval <= 1.00001 )
  {
   horizontalCoarseGridInterval = 2.0;
   return;
  }
 }
 else
 {
  jassert(newGridInterval > 0.000001); 
   // grid-intervals must be > 0
  if( newGridInterval <= 0.000001 )
   return;
 }

 horizontalCoarseGridIsVisible = shouldBeVisible;
 horizontalCoarseGridInterval  = newGridInterval;

 updateBackgroundImage();
}

void CoordinateSystem::setHorizontalFineGrid(double newGridInterval, 
                                             bool   shouldBeVisible)
{
 if( isLogScaledY )
 {
  jassert(newGridInterval > 1.00001);
   // for logarithmic scaling, we need the grid-intervals to be > 1
  if( newGridInterval <= 1.00001 )
  {
   horizontalFineGridInterval = pow(2.0, 1.0/3.0);
   return;
  }
 }
 else
 {
  jassert(newGridInterval > 0.000001); 
   // grid-intervals must be > 0
  if( newGridInterval <= 0.000001 )
   return;
 }

 horizontalFineGridIsVisible = shouldBeVisible;
 horizontalFineGridInterval  = newGridInterval;

 updateBackgroundImage();
}

void CoordinateSystem::setVerticalCoarseGrid(double newGridInterval, 
                                             bool   shouldBeVisible)
{
 if( isLogScaledX )
 {
  jassert(newGridInterval > 1.00001);
   // for logarithmic scaling, we need the grid-intervals to be > 1
  if( newGridInterval <= 1.00001 )
  {
   verticalCoarseGridInterval = 2.0;
   return;
  }
 }
 else
 {
  jassert(newGridInterval > 0.000001); 
   // grid-intervals must be > 0
  if( newGridInterval <= 0.000001 )
   return;
 }

 verticalCoarseGridIsVisible = shouldBeVisible;
 verticalCoarseGridInterval  = newGridInterval;

 updateBackgroundImage();
}

void CoordinateSystem::setVerticalFineGrid(double newGridInterval, 
                                           bool   shouldBeVisible)
{
 if( isLogScaledX )
 {
  jassert(newGridInterval > 1.00001);
   // for logarithmic scaling, we need the grid-intervals to be > 1
  if( newGridInterval <= 1.00001 )
  {
   verticalFineGridInterval = pow(2.0, 1.0/3.0);
   return;
  }
 }
 else
 {
  jassert(newGridInterval > 0.000001); 
   // grid-intervals must be > 0
  if( newGridInterval <= 0.000001 )
   return;
 }

 verticalFineGridIsVisible = shouldBeVisible;
 verticalFineGridInterval  = newGridInterval;

 updateBackgroundImage();
}


void CoordinateSystem::setRadialCoarseGrid(double newGridInterval, 
                                           bool   shouldBeVisible)
{
 if( isLogScaledRadius )
 {
  jassert(newGridInterval > 1.00001);
   // for logarithmic scaling, we need the grid-intervals to be > 1
  if( newGridInterval <= 1.00001 )
  {
   radialCoarseGridInterval = 2.0;
   return;
  }
 }
 else
 {
  jassert(newGridInterval > 0.000001); 
   // grid-intervals must be > 0
  if( newGridInterval <= 0.000001 )
   return;
 }

 radialCoarseGridIsVisible = shouldBeVisible;
 radialCoarseGridInterval  = newGridInterval;

 updateBackgroundImage();
}

void CoordinateSystem::setRadialFineGrid(double newGridInterval, 
                                         bool   shouldBeVisible)
{
 if( isLogScaledRadius )
 {
  jassert(newGridInterval > 1.00001);
   // for logarithmic scaling, we need the grid-intervals to be > 1
  if( newGridInterval <= 1.00001 )
  {
   radialFineGridInterval = pow(2.0, 1.0/3.0);
   return;
  }
 }
 else
 {
  jassert(newGridInterval > 0.000001); 
   // grid-intervals must be > 0
  if( newGridInterval <= 0.000001 )
   return;
 }

 radialFineGridIsVisible     = shouldBeVisible;
 radialFineGridInterval = newGridInterval;

 updateBackgroundImage();
}

void CoordinateSystem::setAngularCoarseGrid(double newGridInterval, 
                                           bool   shouldBeVisible)
{
 jassert(newGridInterval > 0.000001); 
  // grid-intervals must be > 0
 if( newGridInterval <= 0.000001 )
  return;

 angularCoarseGridIsVisible = shouldBeVisible;
 angularCoarseGridInterval  = newGridInterval;

 updateBackgroundImage();
}

void CoordinateSystem::setAngularFineGrid(double newGridInterval, 
                                         bool   shouldBeVisible)
{
 jassert(newGridInterval > 0.000001); 
  // grid-intervals must be > 0
 if( newGridInterval <= 0.000001 )
  return;

 angularFineGridIsVisible     = shouldBeVisible;
 angularFineGridInterval = newGridInterval;

 updateBackgroundImage();
}

void CoordinateSystem::setAngleUnitToDegrees(bool shouldBeInDegrees)
{
 angleIsInDegrees = shouldBeInDegrees;
}

bool CoordinateSystem::isHorizontalCoarseGridVisible() 
{ 
 return horizontalCoarseGridIsVisible; 
}

bool CoordinateSystem::isHorizontalFineGridVisible() 
{ 
 return horizontalFineGridIsVisible; 
}

bool CoordinateSystem::isVerticalCoarseGridVisible() 
{ 
 return verticalCoarseGridIsVisible; 
}

bool CoordinateSystem::isVerticalFineGridVisible() 
{ 
 return verticalFineGridIsVisible; 
}

bool CoordinateSystem::isRadialCoarseGridVisible() 
{ 
 return radialCoarseGridIsVisible; 
}

bool CoordinateSystem::isRadialFineGridVisible() 
{ 
 return radialFineGridIsVisible; 
}

bool CoordinateSystem::isAngularCoarseGridVisible() 
{ 
 return angularCoarseGridIsVisible; 
}

bool CoordinateSystem::isAngularFineGridVisible() 
{ 
 return angularFineGridIsVisible; 
}

double CoordinateSystem::getHorizontalCoarseGridInterval()
{
 return horizontalCoarseGridInterval;
}

double CoordinateSystem::getHorizontalFineGridInterval()
{
 return horizontalFineGridInterval;
}

double CoordinateSystem::getVerticalCoarseGridInterval()
{
 return verticalCoarseGridInterval;
}

double CoordinateSystem::getVerticalFineGridInterval()
{
 return verticalFineGridInterval;
}

double CoordinateSystem::getRadialCoarseGridInterval()
{
 return radialCoarseGridInterval;
}

double CoordinateSystem::getRadialFineGridInterval()
{
 return radialFineGridInterval;
}

double CoordinateSystem::getAngularCoarseGridInterval()
{
 return angularCoarseGridInterval;
}

double CoordinateSystem::getAngularFineGridInterval()
{
 return angularFineGridInterval;
}

void CoordinateSystem::setAxisPositionX(int newAxisPositionX)
{
 if( newAxisPositionX == INVISIBLE ||
     newAxisPositionX == ZERO      ||
     newAxisPositionX == TOP       ||
     newAxisPositionX == BOTTOM       )
 {
  axisPositionX = newAxisPositionX;
  updateBackgroundImage();
 }
}

void CoordinateSystem::setAxisPositionY(int newAxisPositionY)
{
 if( newAxisPositionY == INVISIBLE ||
     newAxisPositionY == ZERO      ||
     newAxisPositionY == LEFT      ||
     newAxisPositionY == RIGHT        )
 {
  axisPositionY = newAxisPositionY;
  updateBackgroundImage();
 }
}

void CoordinateSystem::useLogarithmicScale(bool   shouldBeLogScaledX, 
			                                        bool   shouldBeLogScaledY, 
			                                        double newLogBaseX, 
			                                        double newLogBaseY)
{
 isLogScaledX = shouldBeLogScaledX;
 isLogScaledY = shouldBeLogScaledY;
 logBaseX     = newLogBaseX;
 logBaseY     = newLogBaseY;

 updateScaleFactors();
 updateBackgroundImage();
}

void CoordinateSystem::useLogarithmicScaleX(bool   shouldBeLogScaledX, 
			                                         double newLogBaseX)
{
 isLogScaledX = shouldBeLogScaledX;
 logBaseX     = newLogBaseX;
 updateScaleFactors();
 updateBackgroundImage();
}

void CoordinateSystem::useLogarithmicScaleY(bool   shouldBeLogScaledY, 
			                                         double newLogBaseY)
{
 isLogScaledY = shouldBeLogScaledY;
 logBaseY     = newLogBaseY;
 updateScaleFactors();
 updateBackgroundImage();
}

void CoordinateSystem::setColourScheme(Colour newBackgroundColour, 
                                       Colour newAxesColour,
                                       Colour newCoarseGridColour,
                                       Colour newFineGridColour,
                                       Colour newOutlineColour)
{
 backgroundColour = newBackgroundColour;
 axesColour       = newAxesColour;
 coarseGridColour = newCoarseGridColour;
 fineGridColour   = newFineGridColour;
 outlineColour    = newOutlineColour;

 updateBackgroundImage();
}

//----------------------------------------------------------------------------
void CoordinateSystem::paint(juce::Graphics &g)
{
 if( backgroundImage.isValid())
 {
  g.drawImage(backgroundImage, 0, 
                               0, 
                               getWidth(), 
                               getHeight(), 
                               0, 
                               0, 
                               backgroundImage.getWidth(), 
                               backgroundImage.getHeight(), 
                               false);
 }
 else
  g.fillAll(Colours::red);
}

void CoordinateSystem::updateBackgroundImage()
{
 if( getWidth() < 1 || getHeight() < 1 )
  return;

 // allocate memory for the first time:
 if( backgroundImage.isNull() )
 {
  backgroundImage = Image(Image::RGB, getWidth(), getHeight(), true);

  if( backgroundImage.isNull() )
   return; // memory allocation failed
 }

 // reallocate memory, if necessary (i.e. the size of the component differs
 // from the size of the current image):
 if( backgroundImage.getWidth()  != getWidth()  ||
     backgroundImage.getHeight() != getHeight()    )
 {
  // delete the old and create a new Image-object:
  if( ! backgroundImage.isNull() )
  {
   backgroundImage = Image();
  }
  backgroundImage = Image(Image::RGB, getWidth(), getHeight(), true);

  if( backgroundImage.isNull() )
   return; // memory allocation failed
 }

 // create a graphics object which is associated with the image to perform
 // the drawing-operations
 Graphics g(backgroundImage);

 // fill the background:
 g.fillAll(backgroundColour);

 // draw the grids, if desired:
 if( horizontalFineGridIsVisible )
  drawHorizontalGrid(g, horizontalFineGridInterval, isLogScaledY, 
                     fineGridColour, 1.0f);
 if( verticalFineGridIsVisible )
  drawVerticalGrid(g, verticalFineGridInterval, isLogScaledX, 
                   fineGridColour, 1.0f);
 if( radialFineGridIsVisible )
  drawRadialGrid(g, radialFineGridInterval, isLogScaledRadius, 
                 fineGridColour, 1.0f);
 if( angularFineGridIsVisible )
  drawAngularGrid(g, angularFineGridInterval, fineGridColour, 1.0f);

 if( horizontalCoarseGridIsVisible )
  drawHorizontalGrid(g, horizontalCoarseGridInterval, isLogScaledY, 
                     coarseGridColour, 1.0f);
 if( verticalCoarseGridIsVisible )
  drawVerticalGrid(g, verticalCoarseGridInterval, isLogScaledX, 
                   coarseGridColour, 1.0f);
 if( radialCoarseGridIsVisible )
  drawRadialGrid(g, radialCoarseGridInterval, isLogScaledRadius, 
                 coarseGridColour, 1.0f);
 if( angularCoarseGridIsVisible )
  drawAngularGrid(g, angularCoarseGridInterval, coarseGridColour, 1.0f);

 // draw the coordinate system:
 if( axisPositionX != NO_ANNOTATION )
  drawAxisX(g);
 if( axisPositionY != NO_ANNOTATION )
  drawAxisY(g);

 // draw the labels on the axes:
 if( labelPositionX != NO_ANNOTATION )
  drawAxisLabelX(g);
 if( labelPositionY != NO_ANNOTATION )
  drawAxisLabelY(g);

 // draw the values on the axes:
 if( axisValuesPositionX != NO_ANNOTATION )
  drawAxisValuesX(g);
 if( axisValuesPositionY != NO_ANNOTATION )
  drawAxisValuesY(g);

 // draw an outlining rectangle:
 g.setColour(outlineColour);
 g.drawRect(0, 0, getWidth(), getHeight(), 1);

 // trigger a repaint:
 repaint();

}

//----------------------------------------------------------------------------

void CoordinateSystem::transformToComponentsCoordinates(double &x, double &y)
{
 // transform the x,y values to coordinates inside this component:
 if( isLogScaledX )
 {
  jassert( x > 0.0 && range.getMinX() > 0.0 );
   // caught a logarithm of a non-positive number
  if( x <= 0.0 || range.getMinX() <= 0.0 )
   return;

  x = pixelsPerIntervalX * logB((x/range.getMinX()), logBaseX);
 }
 else
 {
  x -= range.getMinX();	// shift origin left/right
  x *= scaleX;	         // scale to fit width
 }

 if( isLogScaledY )
 {
  jassert( y > 0.0 && range.getMinY() > 0.0 );
   // caught a logarithm of a non-positive number
  if( y <= 0.0 || range.getMinY() <= 0.0 )
   return;

  y = pixelsPerIntervalY * logB((y/range.getMinY()), logBaseY);
 }
 else
 {
  y -= range.getMinY();	// shift origin up/down
  y *= scaleY;	         // scale to fit height
 }

 y  = getHeight()-y;	   // invert (pixels begin at top-left)
}

void CoordinateSystem::transformToComponentsCoordinates(float &x, float &y)
{
 // transform the x,y values to coordinates inside this component:
 if( isLogScaledX )
 {
  jassert( x > 0.f && range.getMinX() > 0.0 );
   // caught a logarithm of a non-positive number
  if( x <= 0.f || range.getMinX() <= 0.0 )
   return;

  x = (float) (pixelsPerIntervalX * logB((x/range.getMinX()), logBaseX));
 }
 else
 {
  x -= (float) range.getMinX();	// shift origin left/right
  x *= (float) scaleX;	         // scale to fit width
 }

 if( isLogScaledY )
 {
  jassert( y > 0.f && range.getMinY() > 0.0 );
   // caught a logarithm of a non-positive number
  if( y <= 0.f || range.getMinY() <= 0.0 )
   return;

  y = (float) (pixelsPerIntervalY * logB((y/range.getMinY()), logBaseY));
 }
 else
 {
  y -= (float) range.getMinY();	// shift origin up/down
  y *= (float) scaleY;	         // scale to fit height
 }

 y  = (float) (getHeight()-y);	 // invert (pixels begin at top-left)
}

void CoordinateSystem::transformFromComponentsCoordinates(double &x, double &y)
{
 if( isLogScaledX )
 {
  x = range.getMinX() * pow(logBaseX, (x/pixelsPerIntervalX));
 }
 else
 {
  x /= scaleX;             // scale to fit width
  x += range.getMinX();    // shift origin left/right
 }

 if( isLogScaledY )
 {
  y = range.getMinY() * pow(logBaseY, (y/pixelsPerIntervalY));
 }
 else
 {
  y  = getHeight()-y; 
  y /= scaleY;             // scale to fit height
  y += range.getMinY();    // shift origin up/down
 }
}

void CoordinateSystem::transformFromComponentsCoordinates(float &x, float &y)
{
 if( isLogScaledX )
 {
  x = (float) (range.getMinX() * pow(logBaseX, (x/pixelsPerIntervalX)));
 }
 else
 {
  x /= (float) scaleX;             // scale to fit width
  x += (float) range.getMinX();    // shift origin left/right
 }

 if( isLogScaledY )
 {
  y = (float) (range.getMinY() * pow(logBaseY, (y/pixelsPerIntervalY)));
 }
 else
 {
  y  = (float) (getHeight()-y); 
  y /= (float) scaleY;             // scale to fit height
  y += (float) range.getMinY();    // shift origin up/down
 }
}



//----------------------------------------------------------------------------

void CoordinateSystem::drawHorizontalGrid(Graphics &g, double interval, 
                                          bool exponentialSpacing, 
                                          Colour gridColour, 
                                          float lineThickness)
{
 if( exponentialSpacing == true )
 {
  jassert( interval >= 1.00001 );
   // grid spacing must be > 1 for exponentially spaced grid-lines
  if( interval < 1.00001 )
   return;
 }
 else
 {
  jassert( interval >= 0.000001 );
   // grid spacing must be > 0
  if( interval < 0.000001 )
   return;
 }

 g.setColour(gridColour);

 long	  i;
 double	startX, endX, startY, endY;
 double accumulator;

 if( exponentialSpacing ) // draw grid with exponentially spaced lines
 {
  accumulator = interval*range.getMinY();
  while( accumulator < range.getMaxY() )
  {
   startX = range.getMinX();
   endX   = range.getMaxX();
   startY = accumulator;
   endY   = accumulator;

   // transform:
   transformToComponentsCoordinates(startX, startY);
   transformToComponentsCoordinates(endX, endY);

   // draw:
   g.drawLine((float)startX, (float)startY, (float)endX, (float)endY, 
              lineThickness);

   accumulator *= interval;
  }
 }
 else // draw grid with linearly spaced lines
 {
  i = 0;
  while( i*interval < range.getMaxY() )
  {
   startX = range.getMinX();
   endX   = range.getMaxX();
   startY = i*interval;
   endY   = i*interval;

   // transform:
   transformToComponentsCoordinates(startX, startY);
   transformToComponentsCoordinates(endX, endY);

   // draw:
   g.drawLine((float)startX, (float)startY, (float)endX, (float)endY, 
              lineThickness);

   i++;
  }
  i = 1;
  while( -i*interval > range.getMinY() )
  {
   startX = range.getMinX();
   endX   = range.getMaxX();
   startY = -i*interval;
   endY   = -i*interval;

   // transform:
   transformToComponentsCoordinates(startX, startY);
   transformToComponentsCoordinates(endX, endY);

   // draw:
   g.drawLine((float)startX, (float)startY, (float)endX, (float)endY, 
              lineThickness);

   i++;
  } // end while
 } // end else
}

void CoordinateSystem::drawVerticalGrid(Graphics &g, double interval, 
                                        bool exponentialSpacing, 
                                        Colour gridColour, 
                                        float lineThickness)
{
 if( exponentialSpacing == true )
 {
  jassert( interval >= 1.00001 );
   // grid spacing must be > 1 for exponentially spaced grid-lines
  if( interval < 1.00001 )
   return;
 }
 else
 {
  jassert( interval >= 0.000001 );
   // grid spacing must be > 0
  if( interval < 0.000001 )
   return;
 }

 g.setColour(gridColour);

 int    	i; 
 double	 startX, endX, startY, endY;
 double  accumulator;

 if( exponentialSpacing ) // draw grid with exponentially spaced lines
 {
  accumulator = interval*range.getMinX();
  while( accumulator < range.getMaxX() )
  {
   startX = accumulator;
   endX   = accumulator;
   startY = range.getMinY();
   endY   = range.getMaxY();

   // transform:
   transformToComponentsCoordinates(startX, startY);
   transformToComponentsCoordinates(endX, endY);

   // draw:
   g.drawLine((float)startX, (float)startY, (float)endX, (float)endY, 
              lineThickness);

   accumulator *= interval;
  }
 }
 else // draw grid with linearly spaced lines
 {
  // draw vertical lines:
  i = 0;
  while( i*interval < range.getMaxX() )
  {
   startX = i*interval;
   endX   = i*interval;
   startY = range.getMinY();
   endY   = range.getMaxY();

   // transform:
   transformToComponentsCoordinates(startX, startY);
   transformToComponentsCoordinates(endX, endY);

   // draw:
   g.drawLine((float)startX, (float)startY, (float)endX, (float)endY, 
              lineThickness);

   i++;
  }
  i = 1;
  while( -i*interval > range.getMinX() )
  {
   startX = -i*interval;
   endX   = -i*interval;
   startY = range.getMinY();
   endY   = range.getMaxY();

   // transform:
   transformToComponentsCoordinates(startX, startY);
   transformToComponentsCoordinates(endX, endY);

   // draw:
   g.drawLine((float)startX, (float)startY, (float)endX, (float)endY, 
              lineThickness);

   i++;
  } // end while
 } // end else
}

void CoordinateSystem::drawRadialGrid(Graphics &g, double interval, 
                                      bool exponentialSpacing, 
                                      Colour gridColour, 
                                      float lineThickness)
{
 if( exponentialSpacing == true )
 {
  jassert( interval >= 1.00001 );
   // grid spacing must be > 1 for exponentially spaced grid-lines
  if( interval < 1.00001 )
   return;
 }
 else
 {
  jassert( interval >= 0.000001 );
   // grid spacing must be > 0
  if( interval < 0.000001 )
   return;
 }

 g.setColour(gridColour);

 // calculate the radius of the largest circle to be drawn:
 double xTmp = jmax(fabs(range.getMinX()), fabs(range.getMaxX()) );
 double yTmp = jmax(fabs(range.getMinY()), fabs(range.getMaxY()) );
 double maxRadius = sqrt(xTmp*xTmp + yTmp*yTmp);

 // calculate the center-coordinates of the circles in terms of components
 // coordinates:
 double centerX = 0.0;
 double centerY = 0.0;
 transformToComponentsCoordinates(centerX, centerY);

 // draw the circles:
 int    i       = 1;
 double radius  = interval;
 double xScaler = getWidth()  / (range.getMaxX()-range.getMinX());
 double yScaler = getHeight() / (range.getMaxY()-range.getMinY());
 double xL, xR, yT, yB;
 while( radius <= maxRadius )
 {
  // draw the circle (may deform to an ellipse depending on the scaling of the 
  // axes):
  xL = centerX - xScaler*radius;
  xR = centerX + xScaler*radius;
  yT = centerY - yScaler*radius;
  yB = centerY + yScaler*radius;
  g.drawEllipse((float)xL, (float)yT, (float)(xR-xL),(float)(yB-yT), 
                lineThickness);

  // calculate the next radius (in system-coordinates)
  i++;
  radius = interval * (double) i;
 }
}

void CoordinateSystem::drawAngularGrid(Graphics &g, double interval,
                                       Colour gridColour, 
                                       float lineThickness)
{
 g.setColour(gridColour);

 double angleIntervalInRadiant;
 if( angleIsInDegrees )
  angleIntervalInRadiant = interval*(double_Pi/180.0);
 else
  angleIntervalInRadiant = interval;

 double angle = 0.0;
 double startX, endX, startY, endY;
 int    i     = 0;
 while( angle <= double_Pi )
 {
  endX   = cos(angle);
  endY   = sin(angle);
  startX = -endX;
  startY = -endY;

  // prolong the lines to be drawn, until they span the whole range of the
  // coordinate-system:
  if( fabs(angle) < 0.00001 ) // equality check with margin
  {
   startX = range.getMinX();
   endX   = range.getMaxX();
   startY = 0.0;
   endY   = 0.0;
  }
  else if( fabs(angle-0.5*double_Pi) < 0.00001 ) // equality check with margin
  {
   startX = 0.0;
   endX   = 0.0;
   startY = range.getMinY();
   endY   = range.getMaxY();
  }
  else if( angle <= 0.5*double_Pi &&  endX > 0.00001 &&  endY > 0.00001 ) 
  // end-point is in 1st quadrant, start-point in 3rd quadrant
  {
   while( startX > range.getMinX() ||
          endX   < range.getMaxX() ||
          startY > range.getMinY() ||
          endY   < range.getMaxY()    )
   {
    startX *= 2.0;
    startY *= 2.0;
    endX   *= 2.0;
    endY   *= 2.0;
   }
  }
  else if( angle  > 0.5*double_Pi && -endX > 0.00001 &&  endY > 0.00001 )
  // end-point is in 2nd quadrant, start-point in 4th quadrant
  {
   while( startX < range.getMaxX() ||
          endX   > range.getMinX() ||
          startY > range.getMinY() ||
          endY   < range.getMaxY()    )
   {
    startX *= 2.0;
    startY *= 2.0;
    endX   *= 2.0;
    endY   *= 2.0;
   }
  }

  // transform:
  transformToComponentsCoordinates(startX, startY);
  transformToComponentsCoordinates(endX, endY);

  // draw:
  g.drawLine((float)startX, (float)startY, (float)endX, (float)endY, 
             lineThickness);

  i++;
  angle = angleIntervalInRadiant * (double) i;
 }
}

//----------------------------------------------------------------------------
void CoordinateSystem::drawAxisX(juce::Graphics &g)
{
 if( isLogScaledX == true )
 {
  jassert( verticalCoarseGridInterval >= 1.00001 );
  if( verticalCoarseGridInterval < 1.00001 )
   return;
 }
 else
 {
  jassert( verticalCoarseGridInterval >= 0.000001 );
   // grid spacing must be > 0
  if( verticalCoarseGridInterval < 0.000001 )
   return;
 }

 if( axisPositionX == INVISIBLE ) 
  return;

 g.setColour(axesColour);

 double startX, endX, startY, endY;

 startX = range.getMinX();
 endX	  = range.getMaxX();
 if( axisPositionX == ZERO )
  startY = 0.0;
 else if( axisPositionX == TOP ) 
  startY = range.getMaxY();
 else if( axisPositionX == BOTTOM ) 
  startY = range.getMinY();
 endY = startY;

 // transform:
 transformToComponentsCoordinates(startX, startY);
 transformToComponentsCoordinates(endX, endY);

 // include some margin for axes at the top and bottom:
 if( axisPositionX == TOP )
 {
  startY += 8;
  endY   += 8;
 }
 else if( axisPositionX == BOTTOM )
 {
  startY -= 8;
  endY   -= 8;
 }

 // draw:
 Line<float> l((float)startX, (float)startY, (float)endX, (float)endY);
 g.drawArrow(l, 2.0, 8.0, 8.0);
}

void CoordinateSystem::drawAxisY(juce::Graphics &g)
{
 if( isLogScaledY == true )
 {
  jassert( horizontalCoarseGridInterval >= 1.00001 );
  if( horizontalCoarseGridInterval < 1.00001 )
   return;
 }
 else
 {
  jassert( horizontalCoarseGridInterval >= 0.000001 );
   // grid spacing must be > 0
  if( horizontalCoarseGridInterval < 0.000001 )
   return;
 }

 if( axisPositionY == INVISIBLE ) 
  return;

 g.setColour(axesColour);

 double startX, endX, startY, endY;

 startY = range.getMinY();
 endY	  = range.getMaxY();
 if( axisPositionY == ZERO )
  startX = 0.0;
 else if( axisPositionY == LEFT ) 
  startX = range.getMinX();
 else if( axisPositionY == RIGHT ) 
  startX = range.getMaxX();
 endX = startX;

 // transform:
 transformToComponentsCoordinates(startX, startY);
 transformToComponentsCoordinates(endX, endY);

 // include some margin for axes at the left and right:
 if( axisPositionY == LEFT )
 {
  startX += 8;
  endX   += 8;
 }
 else if( axisPositionY == RIGHT )
 {
  startX -= 8;
  endX   -= 8;
 }

 // draw:
 Line<float> l((float)startX, (float)startY, (float)endX, (float)endY);
 g.drawArrow(l, 2.0, 8.0, 8.0);
}

//----------------------------------------------------------------------------
void CoordinateSystem::drawAxisLabelX(juce::Graphics &g)
{
 if( axisPositionX == INVISIBLE ) 
  return;

 g.setColour(axesColour);

 double posX, posY;

 // position for the label on the x-axis:
 posX = range.getMaxX();
 if( axisPositionX == ZERO )
  posY = 0;
 else if( axisPositionX == TOP )
  posY = range.getMaxY();
 else if( axisPositionX == BOTTOM )
  posY = range.getMinY();

 // transform coordinates:
 transformToComponentsCoordinates(posX, posY);
 posY += 2;

 // include some margin for axes at the top and bottom:
 if( axisPositionX == TOP )
  posY += 8;
 else if( axisPositionX == BOTTOM || labelPositionX == ABOVE_AXIS )
  posY -= 28;

 g.drawText(labelX, (int) posX - 68, (int) posY, 64, 16, 
            Justification::centredRight, false);
}

void CoordinateSystem::drawAxisLabelY(juce::Graphics &g)
{
 if( axisPositionY == INVISIBLE )
  return;

 g.setColour(axesColour);

 double posX, posY;

 if( axisPositionY == ZERO )
  posX = 0.0;
 else if( axisPositionY == LEFT ) 
  posX = range.getMinX();
 else if( axisPositionY == RIGHT ) 
  posX = range.getMaxX();

 posY  = range.getMaxY();

 // transform coordinates:
 transformToComponentsCoordinates(posX, posY);

 // include some margin for axes at the left and right:

 if( axisPositionY == LEFT || labelPositionY == RIGHT_TO_AXIS )
 {
  posX += 16;
  g.drawText(labelY, (int) posX, (int) posY, 512, 16, 
             Justification::centredLeft, false);
 }
 else
 {
  if( axisPositionY == ZERO )
   posX -=8;
  else
   posX -= 16;
  posX -= 512;
  g.drawText(labelY, (int) posX, (int) posY, 512, 16, 
             Justification::centredRight, false);

 }
}

//----------------------------------------------------------------------------
void CoordinateSystem::drawAxisValuesX(juce::Graphics &g)
{
 if( isLogScaledX == true )
 {
  jassert( verticalCoarseGridInterval >= 1.00001 );
  if( verticalCoarseGridInterval < 1.00001 )
   return;
 }
 else
 {
  jassert( verticalCoarseGridInterval >= 0.000001 );
   // grid spacing must be > 0
  if( verticalCoarseGridInterval < 0.000001 )
   return;
 }

 if( axisPositionX == INVISIBLE )
  return;

 g.setColour(axesColour);

 long	  i;
 double	posX, posY, value;
 double accumulator;
 String numberString;

 // draw values on x-axis:
 if(isLogScaledX)
 {
  posX	       = verticalCoarseGridInterval*range.getMinX();
  accumulator = verticalCoarseGridInterval*range.getMinX();
  while( accumulator < range.getMaxX() )
  {
   posX = value = accumulator;
   if( isLogScaledY )
    posY = range.getMinY();
   else
   {
    if( axisPositionX == ZERO )
     posY = 0;
    else if( axisPositionX == TOP )
     posY = range.getMaxY();
    else if( axisPositionX == BOTTOM )
     posY = range.getMinY();
   }

   // transform:
   transformToComponentsCoordinates(posX, posY);

   // include some margin for axes at the top and bottom:
   if( axisPositionX == TOP )
    posY += 8;
   if( axisPositionX == BOTTOM )
    posY -= 8;

   // draw a small line:
   g.drawLine((float)posX, (float)(posY-4.0), (float)posX, (float)(posY+4.0), 
              1.0);

   // draw number:
   numberString = stringConversionFunctionX(value);
   if( axisValuesPositionX == ABOVE_AXIS || axisPositionX == BOTTOM )
    g.drawText(numberString, (int)posX-32, (int)posY-20, 64, 16, 
	      juce::Justification::centred, false);
   else
    g.drawText(numberString, (int)posX-32, (int)posY+4, 64, 16, 
	      juce::Justification::centred, false);

   accumulator *= verticalCoarseGridInterval;
  }
 }
 else	 // x is linerarly scaled
 {
  if( axisPositionY == LEFT  || 
      axisPositionY == RIGHT || 
      axisPositionY == INVISIBLE)
   i = 0;
  else
   i = 1;
  while( i*verticalCoarseGridInterval < range.getMaxX() )
  {
   posX = value = i*verticalCoarseGridInterval;
           // "value" will not be transformed
   if( isLogScaledY )
    posY = range.getMinY();
   else
   {
    if( axisPositionX == ZERO )
     posY = 0;
    else if( axisPositionX == TOP )
     posY = range.getMaxY();
    else if( axisPositionX == BOTTOM )
     posY = range.getMinY();
   }

   // transform coordinates:
   transformToComponentsCoordinates(posX, posY); 

   // include some margin for axes at the top and bottom:
   if( axisPositionX == TOP )
    posY += 8;
   if( axisPositionX == BOTTOM )
    posY -= 8;

   // draw a small line:
   g.drawLine((float)posX, (float)(posY-4.0), (float)posX, (float)(posY+4.0), 
              1.0);

   // draw the number:
   numberString = stringConversionFunctionX(value);
   if( axisValuesPositionX == ABOVE_AXIS || axisPositionX == BOTTOM )
    g.drawText(numberString, (int)posX-32, (int)posY-20, 64, 16, 
	      juce::Justification::centred, false);
   else
    g.drawText(numberString, (int)posX-32, (int)posY+4, 64, 16, 
	      juce::Justification::centred, false);

   i++;
  }
  i = 1;
  while( -i*verticalCoarseGridInterval > range.getMinX() )
  {
   posX = value = -i*verticalCoarseGridInterval; 
           // "value" will not be transformed
   if( isLogScaledY )
    posY = range.getMinY();
   else
   {
    if( axisPositionX == ZERO )
     posY = 0;
    else if( axisPositionX == TOP )
     posY = range.getMaxY();
    else if( axisPositionX == BOTTOM )
     posY = range.getMinY();
   }

   // transform coordinates:
   transformToComponentsCoordinates(posX, posY);

   // include some margin for axes at the top and bottom:
   if( axisPositionX == TOP )
    posY += 8;
   if( axisPositionX == BOTTOM )
    posY -= 8;

   // draw a small line:
   g.drawLine((float)posX, (float)(posY-4.0), (float)posX, (float)(posY+4.0), 
              1.0);

   // draw the number:
   numberString = stringConversionFunctionX(value);
   if( axisValuesPositionX == ABOVE_AXIS || axisPositionX == BOTTOM )
    g.drawText(numberString, (int)posX-32, (int)posY-20, 64, 16, 
	      juce::Justification::centred, false);
   else
    g.drawText(numberString, (int)posX-32, (int)posY+4, 64, 16, 
	      juce::Justification::centred, false);

   i++;
  }
 }
}

void CoordinateSystem::drawAxisValuesY(juce::Graphics &g)
{
 if( isLogScaledY == true )
 {
  jassert( horizontalCoarseGridInterval >= 1.00001 );
  if( horizontalCoarseGridInterval < 1.00001 )
   return;
 }
 else
 {
  jassert( horizontalCoarseGridInterval >= 0.000001 );
   // grid spacing must be > 0
  if( horizontalCoarseGridInterval < 0.000001 )
   return;
 }

 if( axisPositionY == INVISIBLE )
  return;

 g.setColour(axesColour);

 long	  i;
 double	posX, posY, value;
 double accumulator;
 String numberString;

 // draw values on y-axis:
 if(isLogScaledY)
 {
  posY	       = horizontalCoarseGridInterval*range.getMinY();
  accumulator = horizontalCoarseGridInterval*range.getMinY();
  while( accumulator < range.getMaxX() )
  {
   if( isLogScaledX )
    posX = range.getMinX();
   else
   {
    if( axisPositionY == ZERO )
     posX = 0.0;
    else if( axisPositionY == LEFT ) 
     posX = range.getMinX();
    else if( axisPositionY == RIGHT ) 
     posX = range.getMaxX();
   }
   posY = value = accumulator;

   // transform:
   transformToComponentsCoordinates(posX, posY);

   // include some margin for axes at the left and right:
   if( axisPositionY == LEFT )
    posX += 8;
   else if( axisPositionY == RIGHT )
    posX -= 8;

   // draw a small line:
   g.drawLine((float)(posX-4.0), (float)posY, (float)(posX+4.0), (float)posY, 
              1.0);

   // draw number:
   numberString = stringConversionFunctionY(value);
   if( axisValuesPositionY == LEFT_TO_AXIS )
    g.drawText(numberString, (int)posX-25, (int)posY-10, 20, 20, 
	       juce::Justification::centredRight, false);
   else
    g.drawText(numberString, (int)posX+4, (int)posY-10, 20, 20, 
	       juce::Justification::centredLeft, false);

   accumulator *= horizontalCoarseGridInterval;
  }
 }
 else // y is linearly scaled
 {
  if( axisPositionX == TOP    || 
      axisPositionX == BOTTOM || 
      axisPositionX == INVISIBLE )
   i = 0;
  else
   i = 1;
  while( i*horizontalCoarseGridInterval < range.getMaxY() )
  {
   if( isLogScaledX )
    posX = range.getMinX();
   else
   {
    if( axisPositionY == ZERO )
     posX = 0.0;
    else if( axisPositionY == LEFT ) 
     posX = range.getMinX();
    else if( axisPositionY == RIGHT ) 
     posX = range.getMaxX();
   }
   posY = value = i*horizontalCoarseGridInterval; 
           // "value" will not be transformed

   // transform coordinates:
   transformToComponentsCoordinates(posX, posY);

   // include some margin for axes at the left and right:
   if( axisPositionY == LEFT )
    posX += 8;
   else if( axisPositionY == RIGHT )
    posX -= 8;

   // draw a small line:
   g.drawLine((float)(posX-4.0), (float)posY, (float)(posX+4.0), (float)posY, 
              1.0);

   // draw number:
   numberString = stringConversionFunctionY(value);
   if( axisValuesPositionY == RIGHT_TO_AXIS || axisPositionY == LEFT )
    g.drawText(numberString, (int)posX+8, (int)posY-10, 64, 20, 
	       juce::Justification::centredLeft, false);
   else
    g.drawText(numberString, (int)posX-8-64, (int)posY-10, 64, 20, 
	       juce::Justification::centredRight, false);

   i++;
  }
  i = 1;
  while( -i*horizontalCoarseGridInterval > range.getMinY() )
  {
   if( isLogScaledX )
    posX = range.getMinX();
   else
   {
    if( axisPositionY == ZERO )
     posX = 0.0;
    else if( axisPositionY == LEFT ) 
     posX = range.getMinX();
    else if( axisPositionY == RIGHT ) 
     posX = range.getMaxX();
   }
   posY = value = -i*horizontalCoarseGridInterval; // "value" will not be transformed

   // transform coordinates:
   transformToComponentsCoordinates(posX, posY);

   // include some margin for axes at the left and right:
   if( axisPositionY == LEFT )
    posX += 8;
   else if( axisPositionY == RIGHT )
    posX -= 8;

   // draw a small line:
   g.drawLine((float)(posX-4.0), (float)posY, (float)(posX+4.0), (float)posY, 
              1.0);

   // draw number:
   numberString = stringConversionFunctionY(value);
   if( axisValuesPositionY == RIGHT_TO_AXIS || axisPositionY == LEFT )
    g.drawText(numberString, (int)posX+8, (int)posY-10, 64, 20, 
	       juce::Justification::centredLeft, false);
   else
    g.drawText(numberString, (int)posX-8-64, (int)posY-10, 64, 20, 
	       juce::Justification::centredRight, false);

   i++;
  }  // end while
 }
}

void CoordinateSystem::updateScaleFactors()
{
 if( !isLogScaledX )
  scaleX = getWidth()  / (range.getMaxX()-range.getMinX()); 
  // scaling factor for linear plots
 else
 {
  jassert( ((range.getMaxX()/range.getMinX()) > 0.0) );
   // caught a logarithm of a non-positive number, make sure that the 
   // minimum and the maximum for the x-coordinate are both strictly positive
   // for logarithmic axis-scaling

  if( (range.getMaxX()/range.getMinX()) > 0.0 )
   pixelsPerIntervalX = getWidth()/logB((range.getMaxX()/range.getMinX()), 
                                        logBaseX);
    // the number of pixels per interval for logarithmic plots - for
    // logBase==2 this is the number of pixels per octave:
  else
   pixelsPerIntervalX = 50; // some arbitrary fallback-value
 }
 if( !isLogScaledY )
  scaleY = getHeight() / (range.getMaxY()-range.getMinY());
 else
 {
  jassert( ((range.getMaxY()/range.getMinY()) > 0.0) );
   // caught a logarithm of a non-positive number, make sure that the 
   // minimum and the maximum for the y-coordinate are both strictly positive
   // for logarithmic axis-scaling

  if( (range.getMaxY()/range.getMinY()) > 0.0 )
   pixelsPerIntervalY = getHeight()/logB((range.getMaxY()/range.getMinY()), 
                                         logBaseY);
  else
   pixelsPerIntervalY = 50; // some arbitrary fallback-value
 }
}

//-----------------------------------------------------------------------------
// state-management:

XmlElement* CoordinateSystem::getStateAsXml(const String& stateName) const
{
 XmlElement* xmlState = new XmlElement(stateName); 
  // the XmlElement which stores all the releveant state-information

 xmlState->setAttribute(String("MinX"), range.getMinX());
 xmlState->setAttribute(String("MaxX"), range.getMaxX());
 xmlState->setAttribute(String("MinY"), range.getMinY());
 xmlState->setAttribute(String("MaxY"), range.getMaxY());

 xmlState->setAttribute(String("HorizontalCoarseGridIsVisible"),    
                        horizontalCoarseGridIsVisible);
 xmlState->setAttribute(String("HorizontalCoarseGridInterval"),
                        horizontalCoarseGridInterval);
 xmlState->setAttribute(String("HorizontalFineGridIsVisible"),      
                        horizontalFineGridIsVisible);
 xmlState->setAttribute(String("HorizontalFineGridInterval"),  
                        horizontalFineGridInterval); 
 xmlState->setAttribute(String("VerticalCoarseGridIsVisible"),    
                        verticalCoarseGridIsVisible);
 xmlState->setAttribute(String("VerticalCoarseGridInterval"),
                        verticalCoarseGridInterval);
 xmlState->setAttribute(String("VerticalFineGridIsVisible"),      
                        verticalFineGridIsVisible);
 xmlState->setAttribute(String("VerticalFineGridInterval"),  
                        verticalFineGridInterval);

 return xmlState;
}

bool CoordinateSystem::setStateFromXml(const XmlElement &xmlState)
{
 bool success = true; // should report about success, not used yet

 range.setMinX( xmlState.getDoubleAttribute(String("MinX"),getMinX()) );
 range.setMaxX( xmlState.getDoubleAttribute(String("MaxX"),getMaxX()) );
 range.setMinY( xmlState.getDoubleAttribute(String("MinY"),getMinY()) );
 range.setMaxY( xmlState.getDoubleAttribute(String("MaxY"),getMaxY()) );

 horizontalCoarseGridIsVisible = xmlState.getBoolAttribute(
  String("HorizontalCoarseGridIsVisible"), isHorizontalCoarseGridVisible());
 horizontalCoarseGridInterval = xmlState.getDoubleAttribute(
  String("HorizontalCoarseGridInterval"), getHorizontalCoarseGridInterval());
 horizontalFineGridIsVisible = xmlState.getBoolAttribute(
  String("HorizontalFineGridIsVisible"), isHorizontalFineGridVisible());
 horizontalFineGridInterval = xmlState.getDoubleAttribute(
  String("HorizontalFineGridInterval"), getHorizontalFineGridInterval());
 verticalCoarseGridIsVisible = xmlState.getBoolAttribute(
  String("VerticalCoarseGridIsVisible"), isVerticalCoarseGridVisible());
 verticalCoarseGridInterval = xmlState.getDoubleAttribute(
  String("VerticalCoarseGridInterval"), getVerticalCoarseGridInterval());
 verticalFineGridIsVisible = xmlState.getBoolAttribute(
  String("VerticalFineGridIsVisible"), isVerticalFineGridVisible());
 verticalFineGridInterval = xmlState.getDoubleAttribute(
  String("VerticalFineGridInterval"), getVerticalFineGridInterval());

 updateBackgroundImage();
 return success;
}

END_JUCE_NAMESPACE
