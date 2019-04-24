/*
 ===============================================================================
 
 PreferencesPane.h
  
 
 This file is part of the LUFS Meter audio measurement plugin.
 Copyright 2012-14 by Klangfreund, Samuel Gaehwiler.
 
 -------------------------------------------------------------------------------
 
 The LUFS Meter can be redistributed and/or modified under the terms of the GNU 
 General Public License Version 2, as published by the Free Software Foundation.
 A copy of the license is included with these source files. It can also be found
 at www.gnu.org/licenses.
 
 The LUFS Meter is distributed WITHOUT ANY WARRANTY.
 See the GNU General Public License for more details.
 
 -------------------------------------------------------------------------------
 
 To release a closed-source product which uses the LUFS Meter or parts of it,
 a commercial license is available. Visit www.klangfreund.com/lufsmeter for more
 information.
 
 ===============================================================================
 */

#ifndef __ANIMATED_SIDE_PANEL__
#define __ANIMATED_SIDE_PANEL__

#include "../MacrosAndJuceHeaders.h"

//==============================================================================
/** A self-animated left sided component.
    Especially useful for a plugin, where pop-up windows are not allowed.
    Derive from it to create e.g. a preferences pane.
 */
class AnimatedSidePanel  : public Component,
                           public ButtonListener
{
public:
    AnimatedSidePanel ();
    
    ~AnimatedSidePanel ();
    
    int getWidthWithoutHandle ();
 
    void setCaptionAndIcon (const String& caption, DrawableComposite* icon);
    
    void setBackgroundColour (const Colour& newBackgroundColour);
    
    void setTopLeftPosition (const int x, const int y);

    virtual void paint (Graphics& g) override;
    
    virtual bool hitTest(int x, int y) override;
    
    virtual void resized() override;
    
    /** The ButtonListener method. */
    virtual void buttonClicked (Button* button) override;
    
protected:
    Colour backgroundColour;
    int titleHeight;
    int borderSize;
    int widthWithoutHandle;
    int initialWidth;
    int topRightHandleWidth;
    
private:
    bool panelIsVisible;
    int xPositionWhenHidden;
    DrawableButton showOrHideButton;
    
    /** The caption that will be shown overlayed onto the showOrHideButton,
     * as a child of showOrHideButton. It is set up such that it will bypass
     * clicks to its parent.
     *
     * This is actually a hack, since I could't figure out how to composite
     * a DrawableText and a DrawableComposite (the icon) into one DrawabeComposite
     * to be shown by the DrawableButton.
     */
    Label showOrHideButtonLabel;
};


#endif  // __ANIMATED_SIDE_PANEL__
