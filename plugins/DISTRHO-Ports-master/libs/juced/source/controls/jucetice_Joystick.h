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
*/

#ifndef __JUCETICE_JOYSTICK_HEADER__
#define __JUCETICE_JOYSTICK_HEADER__

class Joystick;


//==============================================================================
/**
    A joystick movement listener
*/
class JoystickListener
{
public:

    virtual ~JoystickListener () {}

    virtual void joystickValueChanged (Joystick* joystick) = 0;
};


//==============================================================================
/**
    A component that acts as a X - Y pad joystick
*/
class Joystick : public Component
{
public:

    //==============================================================================
    /** Creates a Joystick.

        When created, you'll need to set up the Joystick's bounds and ranges with setBounds(),
        setRanges(), etc.
    */
    Joystick ();

    /** Destructor. */
    ~Joystick();


    //==============================================================================
    /**
        Sets the limits that the joystick's value can take.

        @param newHorizontalMinimum        the lowest horizontal value allowed
        @param newHorizontalMaximum        the highest horizontal value allowed
        @param newVerticalMinimum        the lowest vertical value allowed
        @param newVerticalMaximum        the highest vertical value allowed
        @param newInterval                the interval between each steps
    */
    void setRanges (const double newHorizontalMinimum,
                    const double newHorizontalMaximum,
                    const double newVerticalMinimum,
                    const double newVerticalMaximum,
                    const double newInterval = 0);

    /**
        Changes the joystick's current values ( for x and y ).

        This will trigger a callback to any ChangeListener objects that are
        registered, and will synchronously call the valueChanged() method in case
        subclasses want to handle it.

        @param newHorizontalValue       the new horizontal value to set - this will
                                        be restricted by the minimum and maximum
                                        range, and will be snapped to the nearest
                                        interval if one has been set
        @param newVerticalValue         the new vertical value to set - this will
                                        be restricted by the minimum and maximum
                                        range, and will be snapped to the nearest
                                        interval if one has been set
        @param sendUpdateMessage        if false, a change to the value will not
                                        trigger a call to any ChangeListeners or
                                        the valueChanged() method
        @param sendMessageSynchronously if true, then a call to the ChangeListeners
                                        will be made synchronously; if false, it
                                        will be asynchronous
    */
    void setValues (double newHorizontalValue,
                    double newVerticalValue,
                    const bool sendUpdateMessage = true,
                    const bool sendMessageSynchronously = true);


    //==============================================================================
    /** Returns the joystick's current horizontal value. */
    double getHorizontalValue() const throw()    { return current_x+x_min; }

    /** Returns the joystick's current vertical value. */
    double getVerticalValue() const throw()      { return ((y_max-y_min)-current_y)+y_min; }

    /**
        Returns the current maximum horizontal value.

        @see setRanges
    */
    double getHorizontalMaximum() const throw()  { return x_max; }

    /**
        Returns the current minimum horizontal value.

        @see setRanges
    */
    double getHorizontalMinimum() const throw()  { return x_min; }

    /**
        Returns the current maximum vertical value.

        @see setRanges
    */
    double getVerticalMaximum() const throw()    { return y_max; }

    /**
        Returns the current minimum vertical value.
        @see setRanges
    */
    double getVerticalMinimum() const throw()    { return y_min; }

    //==============================================================================
    /**
        Returns the current joystick's values as a string
        in the format "X, Y"
    */
    const String getCurrentValueAsString ();

    //==============================================================================
    /** If this is set to true, then right-clicking on the joystick will pop-up
        a menu to let the user change the way it works.

        By default this is turned off, but when turned on, the menu will include
        things like velocity sensitivity, ...
    */
    void setPopupMenuEnabled (const bool menuEnabled);

    /** Changes the way the the mouse is used when dragging the joystick.

        If true, this will turn on velocity-sensitive dragging, so that
        the faster the mouse moves, the bigger the movement to the slider. This
        helps when making accurate adjustments if the slider's range is quite large.

        If false, the slider will just try to snap to wherever the mouse is.
    */
    void setVelocityBasedMode (const bool velBased);

    //==============================================================================
    /**
        Add a listener to this joystick
    */
    void addListener (JoystickListener* const listener);

    /**
        Remove a listener from this joystick
    */
    void removeListener (JoystickListener* const listener);

    //==============================================================================
    /** Callback to indicate that the user is about to start dragging the slider.
    */
    virtual void startedDragging();

    /** Callback to indicate that the user has just stopped dragging the slider.
    */
    virtual void stoppedDragging();

    /** Callback to indicate that the user has just moved the slider.
    */
    virtual void valueChanged (double newHorizontalValue,
                               double newVerticalValue);

    //==============================================================================
    /** Changes the inner box offset of the joystick. */
    void setBackgroundColour (const Colour& newBackgroundColour)
    {
        backgroundColour = newBackgroundColour;
    }

    void setInsetColours (const Colour& newInsetLight, const Colour& newInsetDark)
    {
        insetLightColour = newInsetLight;
        insetDarkColour = newInsetDark;
    }

    //==============================================================================
    /** @internal */
    void paint (Graphics &g);
    /** @internal */
    void mouseDown(const MouseEvent& e);
    /** @internal */
    void mouseDrag(const MouseEvent& e);
    /** @internal */
    void mouseUp(const MouseEvent& e);
    /** @internal */
    void mouseMove(const MouseEvent& e);
    /** @internal */
    bool keyPressed (const KeyPress& key);
    /** @internal */
    bool keyStateChanged(const bool keyDown);
    /** @internal */
    void resized();

    //==============================================================================
    juce_UseDebuggingNewOperator

private:

    /** Calculate the x and y ratio based on the width and
        height of the box and the min and max values. */
    void calculateRatio();
    /** Calculate the new snapback values based on the min and max. */
    void calculateSnapBack();
    /** Calculate drawing spot. */
    void calculateDrawingSpot();
    /** Send change to ChangeListeners. */
    void sendChanges();
    /** Takes care of restoring mouse if we have unbounded movement */
    void restoreMouseIfHidden();
    /** Converts a value to a text string */
    const String getTextFromValue (double v);


    double current_x, current_y;
    double x_min, x_max, y_min, y_max;
    double interval;
    double x_snap, y_snap;
    Colour backgroundColour,
           insetDarkColour,
           insetLightColour;

    bool sendChangeOnlyOnRelease : 1,
         holdOnMouseRelease      : 1,
         isVelocityBased         : 1,
         menuEnabled             : 1,
         menuShown               : 1,
         mouseWasHidden          : 1;

    int numDecimalPlaces;
    int draw_x, draw_y;
    int startPressX, startPressY;
    float x_ratio, y_ratio;

    Array<void*> listeners;

    Joystick (const Joystick&);
};


#endif // __JUCETICE_JOYSTICK_HEADER__
