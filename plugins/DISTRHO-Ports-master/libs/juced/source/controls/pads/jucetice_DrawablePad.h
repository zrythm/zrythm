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

    Original code: Reuben a.k.a. Insertpizhere

 ==============================================================================
*/

#ifndef __JUCETICE_DRAWABLEPAD_HEADER__
#define __JUCETICE_DRAWABLEPAD_HEADER__

class DrawablePad  : public Button
{
public:

    //==============================================================================
    DrawablePad (const String& buttonName);
    ~DrawablePad ();

    //==============================================================================
    void setImages (const Drawable* normalImage,
                    const Drawable* overImage = 0,
                    const Drawable* downImage = 0,
                    const Drawable* disabledImage = 0,
                    const Drawable* normalImageOn = 0,
                    const Drawable* overImageOn = 0,
                    const Drawable* downImageOn = 0,
                    const Drawable* disabledImageOn = 0);


    //==============================================================================
    void setBackgroundColours (const Colour& toggledOffColour,
                               const Colour& toggledOnColour);

    const Colour& getBackgroundColour() const throw();

    //==============================================================================
    /** Returns the image that the button is currently displaying. */
    const Drawable* getCurrentImage() const throw();
    const Drawable* getNormalImage() const throw();
    const Drawable* getOverImage() const throw();
    const Drawable* getDownImage() const throw();

    //==============================================================================
    void setHex(bool newhex);
    bool isHex();

    //==============================================================================
    /** @internal */
    void drawButtonBackground (Graphics& g,
                               Button& button,
                               const Colour& backgroundColour,
                               bool isMouseOverButton,
                               bool isButtonDown);

    /** @internal */
    void paintButton (Graphics& g,
                      bool isMouseOverButton,
                      bool isButtonDown);
    /** @internal */
    void resized();

    //==============================================================================
    juce_UseDebuggingNewOperator

    //==============================================================================
    /** @TODO - make protected... use getters and setters instead ! */
    String Label, Description;
    bool showx, showy;
    float x;
    float y;
    float roundness;
    bool showdot;

private:

    //==============================================================================
    void deleteImages();

    //==============================================================================
    //ButtonState buttonState;
    Drawable* normalImage;
    Drawable* overImage;
    Drawable* downImage;
    Drawable* disabledImage;
    Drawable* normalImageOn;
    Drawable* overImageOn;
    Drawable* downImageOn;
    Drawable* disabledImageOn;

    Colour backgroundOff, backgroundOn;
    Path hexpath;
    bool hex;

    //==============================================================================
    DrawablePad (const DrawablePad&);
    const DrawablePad& operator= (const DrawablePad&);
};

#endif
