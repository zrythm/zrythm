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

BEGIN_JUCE_NAMESPACE

//==============================================================================
Joystick::Joystick()
  : Component ("Joystick"),
    current_x (0),
    current_y (0),
    x_min (0),
    x_max (1),
    y_min (0),
    y_max (1),
    backgroundColour (Colours::black),
    sendChangeOnlyOnRelease (false),
    holdOnMouseRelease (false),
    isVelocityBased (false),
    menuEnabled (true),
    menuShown (false),
    mouseWasHidden (false),
    numDecimalPlaces (4)
{
    setOpaque (true);
    setWantsKeyboardFocus (true);

    calculateRatio();
    calculateSnapBack();
}

Joystick::~Joystick()
{
    deleteAllChildren();
}

//==============================================================================
void Joystick::resized()
{
    x_ratio = getWidth() / (float) (x_max - x_min);
    y_ratio = getHeight() / (float) (y_max - y_min);

    calculateDrawingSpot();
}

//==============================================================================
const String Joystick::getCurrentValueAsString()
{
    String text;
    text << getTextFromValue (current_x+x_min) << ", " << getTextFromValue (((y_max-y_min)-current_y)+y_min) ;
    return text;
}

//==============================================================================
void Joystick::setVelocityBasedMode (const bool velBased)
{
    isVelocityBased = velBased;
}

void Joystick::setPopupMenuEnabled (const bool menuEnabled_)
{
    menuEnabled = menuEnabled_;
}

//==============================================================================
void Joystick::setRanges (const double newHorizontalMin,
                          const double newHorizontalMax,
                          const double newVerticalMin,
                          const double newVerticalMax,
                          const double newInt)
{
    bool horizontalChanged = false;
    bool verticalChanged = false;

    if (x_min != newHorizontalMin
        || x_max != newVerticalMax)
    {
        x_min = newHorizontalMin;
        x_max = newHorizontalMax;
        horizontalChanged = true;
    }

    if (y_min != newVerticalMin
        || y_max != newVerticalMax)
    {
        y_min = newVerticalMin;
        y_max = newVerticalMax;
        verticalChanged = true;
    }

    // figure out the number of DPs needed to display all values at this
    // interval setting.
    interval = newInt;
    numDecimalPlaces = 7;
    if (newInt != 0)
    {
        int v = abs ((int) (newInt * 10000000));
        while ((v % 10) == 0)
        {
            --numDecimalPlaces;
            v /= 10;
        }
    }

    if (horizontalChanged || verticalChanged)
        setValues (current_x, current_y, false);

    calculateRatio();
    calculateSnapBack();
    calculateDrawingSpot();
}

void Joystick::setValues (double newHorizontalValue, double newVerticalValue,
                          const bool sendUpdateMessage,
                          const bool sendMessageSynchronously)
{
    if (newHorizontalValue <= x_min || x_max <= x_min)
    {
        newHorizontalValue = x_min;
    }
    else if (newHorizontalValue >= x_max)
    {
        newHorizontalValue = x_max;
    }
    else if (interval > 0)
    {
        newHorizontalValue = x_min + interval * floor ((newHorizontalValue - x_min) / interval + 0.5);
    }

    if (newVerticalValue <= y_min || y_max <= y_min)
    {
        newVerticalValue = y_min;
    }
    else if (newVerticalValue >= y_max)
    {
        newVerticalValue = y_max;
    }
    else if (interval > 0)
    {
        newVerticalValue = y_min + interval * floor ((newVerticalValue - y_min) / interval + 0.5);
    }

    if (current_x != newHorizontalValue)
        current_x = newHorizontalValue;

    if (current_y != newVerticalValue)
        current_y = newVerticalValue;

    if (current_x != newHorizontalValue || current_y != newVerticalValue)
    {
        if (sendUpdateMessage)
        {
            if (sendMessageSynchronously)
                sendChanges();
            else
                jassertfalse; // asyncronous message not implemented !

            valueChanged (current_x, current_y);
        }

        calculateDrawingSpot();
    }
}

//==============================================================================
void Joystick::paint (Graphics& g)
{
    // Draw main section of box ------------------------
    g.setColour (backgroundColour);
    g.fillAll ();

    // Determine the location of the snapback square ---
    int snap_x = roundDoubleToInt (floor (x_snap * x_ratio));
    int snap_y = roundDoubleToInt (floor (y_snap * y_ratio));

    // if at max value, don't go over edge
    if (x_snap == x_min) snap_x++;
    if (y_snap == x_min) snap_y++;
    if (x_snap == (x_max - x_min)) snap_x--;
    if (y_snap == (x_max - x_min)) snap_y--;

    // Draw the snapback square ------------------------
    g.setColour (Colours::lightyellow);
//    g.fillEllipse(snap_x-2,snap_y-2,5,5);
    g.drawEllipse (snap_x - 4, snap_y - 4, 9, 9, 1.0);

    // Draw the main dot -------------------------------
    if (holdOnMouseRelease)
        g.setColour (Colours::lightblue);
    else
        g.setColour (Colours::orange);

    g.fillEllipse (draw_x - 2, draw_y - 2, 5, 5);
    g.drawEllipse (draw_x - 4, draw_y - 4, 9, 9, 1.0);

    // Draw inset bevel --------------------------------
    LookAndFeel_V2::drawBevel (g,
                            0, 0,
                            getWidth(), getHeight(),
                            2,
                            insetDarkColour,
                            insetLightColour,
                            true);
}

//==============================================================================
void Joystick::mouseDown(const MouseEvent& e)
{
    mouseWasHidden = false;

    if (isEnabled())
    {
        if (e.mods.isPopupMenu() && menuEnabled)
        {
            menuShown = true;

            PopupMenu m;
            m.addItem (1, TRANS ("velocity-sensitive mode"), true, isVelocityBased);
            m.addSeparator();

            const int r = m.show();
            if (r == 1)
            {
                setVelocityBasedMode (! isVelocityBased);
            }
            else if (r == 2)
            {
//                setSliderStyle (RotaryHorizontalDrag);
            }
            else if (r == 3)
            {
//                setSliderStyle (RotaryVerticalDrag);
            }
        }
        else if (e.mods.isLeftButtonDown()
                 || e.mods.isMiddleButtonDown())
        {
            menuShown = false;

            // unbound mouse movements
            if (isVelocityBased)
            {
                e.source.enableUnboundedMouseMovement (true, false);
                mouseWasHidden = true;
            }

            startPressX = e.x;
            startPressY = e.y;

            // In case it was a click without a drag, do the drag anyway
            mouseDrag (e);
        }

        // So arrow keys works
        grabKeyboardFocus();
    }
}

void Joystick::mouseDrag (const MouseEvent& e)  //Called continuously while moving mouse
{
    if (isEnabled() && ! menuShown)
    {
        // If shift is down, don't allow changes to the X value
        if (! e.mods.isShiftDown())
        {
            //Determine current X value based on mouse location
            int x = startPressX + e.getDistanceFromDragStartX ();
            if (x < 0)
                current_x = 0;
            else if (x >= getWidth())
                current_x = x_max - x_min;
            else
                current_x = x / x_ratio;
        }

        // If ctrl is down, don't allow changes to Y value
        if (! e.mods.isCtrlDown())
        {
            //Determine current Y value based on mouse location
            int y = startPressY + e.getDistanceFromDragStartY ();
            if (y < 0)
                current_y = 0;
            else if (y > getHeight())
                current_y = y_max - y_min;
            else
                current_y = y / y_ratio;
        }

        // If holding down control key, then set snapback values
        if (e.mods.isLeftButtonDown())
        {
            x_snap = current_x;
            y_snap = current_y;
        }

        // If right left is down, then hold
        if (e.mods.isLeftButtonDown())
            holdOnMouseRelease = true;
        else
            holdOnMouseRelease = false;

        // Send changes and repaint screen
        if (! sendChangeOnlyOnRelease)
        {
            valueChanged (current_x, current_y);
            sendChanges ();
        }

        // update drawing position
        calculateDrawingSpot();

        repaint ();
    }
}


void Joystick::mouseUp (const MouseEvent& e)  //Called when the mouse button is released
{
    if (isEnabled() && ! menuShown)
    {
        // If not holding, then jump to snapback values
        if (! holdOnMouseRelease)
        {
            current_x = x_snap;
            current_y = y_snap;
        }

        // Call user defined callback
        valueChanged (current_x, current_y);

        // Send controllers changes values
        sendChanges ();

        // update drawing position
        calculateDrawingSpot();

        // restore mouse
        restoreMouseIfHidden();

        repaint ();
    }
}

void Joystick::mouseMove (const MouseEvent& e)
{
}

//==============================================================================
bool Joystick::keyPressed (const KeyPress& key)
{
    if (key.isKeyCode (KeyPress::upKey))
    {
        setValues(current_x,current_y-interval);
    }
    else if (key.isKeyCode (KeyPress::downKey))
    {
        setValues(current_x,current_y+interval);
    }
    else if (key.isKeyCode (KeyPress::leftKey))
    {
        setValues(current_x-interval,current_y);
    }
    else if (key.isKeyCode (KeyPress::rightKey))
    {
        setValues(current_x+interval,current_y);
    }
    else
    {
        return Component::keyPressed (key);
    }

    return true;
}

bool Joystick::keyStateChanged(const bool keyDown)
{
    // only forward key events that aren't used by this component
    if (keyDown && 
          ! (KeyPress::isKeyCurrentlyDown (KeyPress::upKey)
          || KeyPress::isKeyCurrentlyDown (KeyPress::leftKey)
          || KeyPress::isKeyCurrentlyDown (KeyPress::downKey)
          || KeyPress::isKeyCurrentlyDown (KeyPress::rightKey)))
    {
        return Component::keyStateChanged(keyDown);
    }

    return false;
}

//==============================================================================
void Joystick::addListener (JoystickListener* const listener)
{
    jassert (listener != 0);
    if (listener != 0)
        listeners.add (listener);
}

void Joystick::removeListener (JoystickListener* const listener)
{
    int index = listeners.indexOf (listener);
    if (index >= 0)
        listeners.remove (index);
}

void Joystick::sendChanges()
{
    for (int i = listeners.size (); --i >= 0;)
        ((JoystickListener*) listeners.getUnchecked (i))->joystickValueChanged (this);
}

//==============================================================================
void Joystick::calculateRatio()
{
    // Calculate the x and y ratio
    x_ratio = getWidth() / (float) (x_max - x_min);
    y_ratio = getHeight() / (float) (y_max - y_min);
}

void Joystick::calculateSnapBack()
{
    // Calculate the new snapback values based on the min and max
//    x_snap = roundDoubleToInt(floor((x_max-x_min)/2.0));
//    y_snap = roundDoubleToInt(floor((y_max-y_min)/2.0));

    x_snap = (x_max - x_min) / 2.0;
    y_snap = (y_max - y_min) / 2.0;

    // If the range is odd, then add 1 to X to calculate center correctly (HACK!)
//    if (roundDoubleToInt(x_max-x_min) % 2 != 0)
//        x_snap++;

    // Set the current location to the snapback
    current_x = x_snap;
    current_y = y_snap;
}

void Joystick::calculateDrawingSpot()
{
    // Determine the current drawing location
    draw_x = roundDoubleToInt (floor (current_x * x_ratio));
    draw_y = roundDoubleToInt (floor (current_y * y_ratio));

    // If at max value, don't go over edge
    if (current_x == x_min) draw_x++;
    if (current_y == y_min) draw_y++;
    if (current_x == (x_max - x_min)) draw_x--;
    if (current_y == (y_max - y_min)) draw_y--;
}

const String Joystick::getTextFromValue (double v)
{
    if (numDecimalPlaces > 0)
        return String (v, numDecimalPlaces);
    else
        return String (roundDoubleToInt (v));
}

//==============================================================================
void Joystick::restoreMouseIfHidden()
{
    if (mouseWasHidden)
    {
        mouseWasHidden = false;

        /*Component* c = Component::getComponentUnderMouse();
        if (c == 0)
            c = this;
        c->enableUnboundedMouseMovement (false);*/

		Point<int> p (getScreenX() + draw_x,
					  getScreenY() + draw_y);
		
        Desktop::setMousePosition (p);
    }
}

//==============================================================================
void Joystick::startedDragging()
{
}

void Joystick::stoppedDragging()
{
}

void Joystick::valueChanged (double x, double y)
{
}
END_JUCE_NAMESPACE

