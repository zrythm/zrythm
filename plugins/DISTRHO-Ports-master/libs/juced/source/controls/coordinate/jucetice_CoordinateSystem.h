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

#ifndef __JUCETICE_COORDINATESYSTEM_HEADER__
#define __JUCETICE_COORDINATESYSTEM_HEADER__

#include "jucetice_CoordinateSystemRange.h"
#include "jucetice_StringTools.h"

//==============================================================================
/**
    This class is a component, intended to be used as base-class for all 
    components that need some some underlying coordinate-system, such as 
    function-plots, XY-pads, etc. It takes care of the coordinate axes, a coarse 
    and a fine grid, conversion between component-coordinates and the coordinates 
    in the desired coordinate-system (which can be lin- or log-scaled).
*/
class CoordinateSystem	:	public Component
{
public:

 enum axisPositions
 {
  INVISIBLE = 0,
  ZERO,
  LEFT,
  RIGHT,
  TOP,
  BOTTOM
 };

 enum axisAnnotationPositions
 {
  NO_ANNOTATION = 0,
  LEFT_TO_AXIS, 
  RIGHT_TO_AXIS,
  ABOVE_AXIS,
  BELOW_AXIS
 };

 CoordinateSystem(const String &name = String("CoordinateSytem"));   
 ///< Constructor.

 virtual ~CoordinateSystem();            
 ///< Destructor.

 virtual void setBounds(int x, int y, int width, int height);
 ///< Overrides the setBounds-function of the component base-class.

 virtual void paint(Graphics &g);
 ///< Overrides the paint-function of the component base-class.

 virtual void setRange(double newMinX, double newMaxX, 
                       double newMinY, double newMaxY);
 /**< Sets the minimum and maximum values for the x-axis and y-axis. For 
      logarithmic x- and/or y-axis-scaling, make sure that the respective
      minimum value is greater than zero! */

 virtual void setRange(CoordinateSystemRange newRange);
 /**< Sets the currently visible range. */

 virtual CoordinateSystemRange getRange() { return range; }
 /**< Returns the currently visible range. */

 virtual void setMinX(double newMinX);
 /**< Sets the minimum value of x. */

 virtual double getMinX() { return range.getMinX(); }
 /**< Returns the minimum value of x. */

 virtual void setMinY(double newMinY);
 /**< Sets the minimum value of y. */

 virtual double getMinY() { return range.getMinY(); }
 /**< Returns the minimum value of y. */

 virtual void setMaxX(double newMaxX);
 /**< Sets the maximum value of x. */

 virtual double getMaxX() { return range.getMaxX(); }
 /**< Returns the maximum value of x. */

 virtual void setMaxY(double newMaxY);
 /**< Sets the maximum value of y. */

 virtual double getMaxY() { return range.getMaxY(); }
 /**< Returns the maximum value of y. */

 virtual CoordinateSystemRange 
  getMaximumMeaningfulRange(double relativeMargin = 10.0);
 /** Returns the maximum range which makes sense for a given plot as 
     CoordinateSystemRange-object. This is supposed to be overriden by 
     subclasses. */

 virtual void setAxisPositionX(int newAxisPositionX);
 /**< Sets the position of the x-axis. For possible values see 
      enum positions. */

 //virtual void setLabelAboveAxisX(bool shouldBeAboveAxis);
 /**< Selects, if the label for the x-axis should be drawn above the axis. */

 virtual void setAxisPositionY(int newAxisPositionY);
 /**< Sets the position of the y-axis. For possible values see 
      enum positions. */

 virtual void setHorizontalCoarseGrid(double newGridInterval, 
                                      bool   shouldBeVisible); 
 /**< Sets the interval and visibility of the horizontal coarse grid. */

 virtual void setHorizontalFineGrid(double newGridInterval, 
                                    bool   shouldBeVisible); 
 /**< Sets the interval and visibility of the horizontal fine grid. */

 virtual void setVerticalCoarseGrid(double newGridInterval, 
                                    bool   shouldBeVisible); 
 /**< Sets the interval and visibility of the vertical coarse grid. */

 virtual void setVerticalFineGrid(double newGridInterval, 
                                  bool   shouldBeVisible); 
 /**< Sets the interval and visibility of the vertical fine grid. */

 virtual void setRadialCoarseGrid(double newGridInterval, 
                                  bool   shouldBeVisible); 
 /**< Sets the interval and visibility of the radial coarse grid. */

 virtual void setRadialFineGrid(double newGridInterval, 
                                bool   shouldBeVisible); 
 /**< Sets the interval and visibility of the radial fine grid. */
                   
 virtual void setAngularCoarseGrid(double newGridInterval, 
                                   bool   shouldBeVisible); 
 /**< Sets the interval and visibility of the angular coarse grid. */

 virtual void setAngularFineGrid(double newGridInterval, 
                                 bool   shouldBeVisible); 
 /**< Sets the interval and visibility of the angular fine grid. */

 virtual void setAngleUnitToDegrees(bool shouldBeInDegrees = true);
 /**< Sets the unit of the angle (as used by the angular grid) to degrees. If
      false, radiant will be assumed. */

 virtual bool isHorizontalCoarseGridVisible();
 /**< Informs, if the horizontal coarse grid is visible. */

 virtual bool isHorizontalFineGridVisible();
 /**< Informs, if the horizontal fine grid is visible. */

 virtual bool isVerticalCoarseGridVisible();
 /**< Informs, if the vertical coarse grid is visible. */

 virtual bool isVerticalFineGridVisible();
 /**< Informs, if the vertical fine grid is visible. */

 virtual bool isRadialCoarseGridVisible();
 /**< Informs, if the radial coarse grid is visible. */

 virtual bool isRadialFineGridVisible();
 /**< Informs, if the radial fine grid is visible. */

 virtual bool isAngularCoarseGridVisible();
 /**< Informs, if the angular coarse grid is visible. */

 virtual bool isAngularFineGridVisible();
 /**< Informs, if the angular fine grid is visible. */

 virtual double getHorizontalCoarseGridInterval();
 /**< Returns the interval of the horizontal coarse grid. */

 virtual double getHorizontalFineGridInterval();
 /**< Returns the interval of the horizontal fine grid. */

 virtual double getVerticalCoarseGridInterval();
 /**< Returns the interval of the vertical coarse grid. */

 virtual double getVerticalFineGridInterval();
 /**< Returns the interval of the vertical fine grid. */

 virtual double getRadialCoarseGridInterval();
 /**< Returns the interval of the radial coarse grid. */

 virtual double getRadialFineGridInterval();
 /**< Returns the interval of the radial fine grid. */

 virtual double getAngularCoarseGridInterval();
 /**< Returns the interval of the angular coarse grid. */

 virtual double getAngularFineGridInterval();
 /**< Returns the interval of the angular fine grid. */

 virtual void useLogarithmicScale(bool   shouldBeLogScaledX, 
                                  bool   shouldBeLogScaledY,
                                  double newLogBaseX = 2.0, 
                                  double newLogBaseY = 2.0);
 /**< Decides if either the x-axis or the y-axis or both should be 
      logarithmically scaled and sets up the base for the logarithms. */

 virtual void useLogarithmicScaleX(bool   shouldBeLogScaledX, 
                                   double newLogBaseX = 2.0);
 /**< Decides, if the x-axis should be logarithmically scaled and sets up the
      base for the logarithm. */

 virtual void useLogarithmicScaleY(bool   shouldBeLogScaledY, 
                                   double newLogBaseY = 2.0);
 /**< Decides, if the y-axis should be logarithmically scaled and sets up the 
      base for the logarithm. */

 virtual void setAxisLabels(const String &newLabelX, 
                            const String &newLabelY, 
                            int newLabelPositionX = ABOVE_AXIS,
                            int newLabelPositionY = RIGHT_TO_AXIS);
 /**< Sets the labels for the axes and their position. */

 virtual void setAxisLabelX(const String &newLabelX, 
                            int newLabelPositionX = ABOVE_AXIS);
 /**< Sets the label for the x-axis and its position. */

 virtual void setAxisLabelY(const String &newLabelY, 
                            int newLabelPositionY = RIGHT_TO_AXIS);
 /**< Sets the label for the y-axis and its position. */

 virtual void setValuesPositionX(int newValuesPositionX);
 /**< Switches x-value annotation between below or above the x-axis 
      (or off). */

 virtual void setValuesPositionY(int newValuesPositionY);
 /**< Switches y-value annotation between left to or right to the y-axis
      (or off). */

 virtual void setStringConversionFunctionX(String (*newConversionFunction) 
                                           (double valueToBeConverted) );
 /**< This function is used to pass a function-pointer. This pointer has to be
      the address of a function which has a double-parameter and a juce::String
      as return-value. The function will be used to convert the values on the 
      x-axis into corresponding strings for display. */

 virtual void setStringConversionFunctionY(String (*newConversionFunction) 
                                           (double valueToBeConverted) );
 /**< see setStringConversionFunctionX() - same for y-axis.  */

 virtual void setColourScheme(Colour newBackgroundColour, 
                              Colour newAxesColour,
                              Colour newCoarseGridColour, 
                              Colour newFineGridColour,
                              Colour newOutLineColour);
 /**< Sets up the colour-scheme for the CoordinateSystem. */

 virtual XmlElement* getStateAsXml(
  const String& stateName = String("CoordinateSystemState")) const;
 /**< Creates an XmlElement from the current state and returns it. */

 virtual bool setStateFromXml(const XmlElement &xmlState);
 /**< Restores a state based on an XmlElement which should have been created
      with the getStateAsXml()-function. */

protected:

 // functions for drawing the background (for internal use):
 virtual void drawHorizontalGrid(Graphics &g, double interval, 
  bool exponentialSpacing, Colour gridColour, float lineThickness);
 /**< Draws an horizontal grid with a given interval in a given colour.*/

 virtual void drawVerticalGrid(Graphics &g, double interval, 
  bool exponentialSpacing, Colour gridColour, float lineThickness);

 virtual void drawRadialGrid(Graphics &g, double interval, 
  bool exponentialSpacing, Colour gridColour, float lineThickness);

 virtual void drawAngularGrid(Graphics &g, double interval, 
  Colour gridColour, float lineThickness);

 virtual void drawAxisX(Graphics &g);
 virtual void drawAxisY(Graphics &g);
 virtual void drawAxisLabelX(Graphics &g);
 virtual void drawAxisLabelY(Graphics &g);
 virtual void drawAxisValuesX(Graphics &g);
 virtual void drawAxisValuesY(Graphics &g);

 virtual void transformToComponentsCoordinates(double &x, double &y);
 /**< Function for converting the x- and y-coordinate values into the 
      corresponding coordinates in the component (double precision version).*/

 virtual void transformToComponentsCoordinates(float &x, float &y);
 /**< Function for converting the x- and y-coordinate values into the 
      corresponding coordinates in the component (single precision version).*/

 virtual void transformFromComponentsCoordinates(double &x, double &y);
 /**< Function for converting the x- and y-coordinate values measured in the
      components coordinate system to the corresponding coordinates of our
      plot (double precision version). */

 virtual void transformFromComponentsCoordinates(float &x, float &y);
 /**< Function for converting the x- and y-coordinate values measured in the
      components coordinate system to the corresponding coordinates of our
      plot (single precision version). */

 CoordinateSystemRange range;
 /**< The range-object for the coordinate system. */

 double scaleX;
 double scaleY;
 double pixelsPerIntervalX;
 double pixelsPerIntervalY;

 int    axisPositionX;
 int    axisPositionY;
 int    labelPositionX;
 int    labelPositionY;
 int    axisValuesPositionX;
 int    axisValuesPositionY;

 String labelX;
 String labelY;

 bool   horizontalCoarseGridIsVisible;
 bool   horizontalFineGridIsVisible;
 bool   verticalCoarseGridIsVisible;
 bool   verticalFineGridIsVisible;
 bool   radialCoarseGridIsVisible;
 bool   radialFineGridIsVisible;
 bool   angularCoarseGridIsVisible;
 bool   angularFineGridIsVisible;

 double horizontalCoarseGridInterval;
 double horizontalFineGridInterval;
 double verticalCoarseGridInterval;
 double verticalFineGridInterval;
 double radialCoarseGridInterval;
 double radialFineGridInterval;
 double angularCoarseGridInterval;
 double angularFineGridInterval;

 bool   angleIsInDegrees;

 bool   isLogScaledX;
 double logBaseX;
 bool   isLogScaledY;
 double logBaseY;
 bool   isLogScaledRadius;
 double logBaseRadius;

 Colour backgroundColour;
 Colour axesColour;
 Colour coarseGridColour;
 Colour fineGridColour;
 Colour outlineColour;

 //int    numDigitsX, numDigitsY;
 /**< Number of decimal digits to be shown in the x- and y-axis annotation */

 String  headline;
 /**< Meant for a plot-headline - not used yet. */

 Image  backgroundImage; 
  // this image will be used for the appearance of the coodinate system, it 
  // will be updated via the updateBackgroundImage()-function when something
  // about the coordinate-system (axes, grid, etc.) changes

 virtual void updateBackgroundImage();
 /**< Updates the image object (re-draws it). Will be called, when something
      about the CoordinateSystem's appearance-settings was changed. */

 virtual void updateScaleFactors();
 /**< Updates the scale-factors which are needed when transforming from the 
      CoordinateSystem's coordinates to Component's coordinates and vice 
      versa. Will be called by setBounds(), setRange() and 
      useLogarithmicScale(). */

 String (*stringConversionFunctionX) (double valueToConvert);
 /**< A pointer to the function which converts a x-value into a juce-string. */

 String (*stringConversionFunctionY) (double valueToConvert);
 /**< A pointer to the function which converts a y-value into a juce-string. */

};

#endif
