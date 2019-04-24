/*
  ==============================================================================

  This file is part of the dRowAudio JUCE module
  Copyright 2004-13 by dRowAudio.

  ------------------------------------------------------------------------------

  dRowAudio is provided under the terms of The MIT License (MIT):

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
  SOFTWARE.

  ==============================================================================
*/



//==============================================================================
CentreAlignViewport::CentreAlignViewport (const String& componentName)
    : Component (componentName),
      scrollBarThickness (0),
      singleStepX (16),
      singleStepY (16),
      showHScrollbar (true),
      showVScrollbar (true),
      verticalScrollBar (true),
      horizontalScrollBar (false),
      shouldCentre (true)
{
    // content holder is used to clip the contents so they don't overlap the scrollbars
    addAndMakeVisible (&contentHolder);
    contentHolder.setInterceptsMouseClicks (true, true);

    addChildComponent (&verticalScrollBar);
    addChildComponent (&horizontalScrollBar);
	
    verticalScrollBar.addListener (this);
    horizontalScrollBar.addListener (this);
	
    setInterceptsMouseClicks (false, true);
    setWantsKeyboardFocus (true);
}

CentreAlignViewport::~CentreAlignViewport()
{
    contentHolder.deleteAllChildren();
}

//==============================================================================
void CentreAlignViewport::visibleAreaChanged (int, int, int, int)
{
}

//==============================================================================
void CentreAlignViewport::setViewedComponent (Component* const newViewedComponent)
{
    if (contentComp.getComponent() != newViewedComponent)
    {
        {
            ScopedPointer<Component> oldCompDeleter (contentComp);
            contentComp = 0;
        }
		
        contentComp = newViewedComponent;
		
        if (contentComp != 0)
        {
            contentComp->setTopLeftPosition (0, 0);
            contentHolder.addAndMakeVisible (contentComp);
            contentComp->addComponentListener (this);
        }
		
        updateVisibleArea();
    }
}

int CentreAlignViewport::getMaximumVisibleWidth() const
{
    return contentHolder.getWidth();
}

int CentreAlignViewport::getMaximumVisibleHeight() const
{
    return contentHolder.getHeight();
}

void CentreAlignViewport::setViewPosition (const int xPixelsOffset, const int yPixelsOffset)
{
    if (contentComp != 0)
	{
		int topX = 0;
		int topY = 0;

		if (contentComp.getComponent()->getBounds().getWidth() < contentHolder.getBounds().getWidth()
			&& shouldCentre)
			topX = roundToInt (contentHolder.getWidth() / 2.0f - contentComp->getWidth() / 2.0f);
		else
			topX = jmax (jmin (0, contentHolder.getWidth() - contentComp->getWidth()), jmin (0, -xPixelsOffset));

		if (contentComp.getComponent()->getBounds().getHeight() < contentHolder.getBounds().getHeight()
			&& shouldCentre)
			topY = roundToInt (contentHolder.getHeight() / 2.0f - contentComp->getHeight() / 2.0f);
		else
			topY = jmax (jmin (0, contentHolder.getHeight() - contentComp->getHeight()), jmin (0, -yPixelsOffset));
				  
		contentComp->setTopLeftPosition (topX, topY);
	}
}

void CentreAlignViewport::setViewPosition (const Point<int>& newPosition)
{
    setViewPosition (newPosition.getX(), newPosition.getY());
}

void CentreAlignViewport::setViewPositionProportionately (const double x, const double y)
{
    if (contentComp != 0)
        setViewPosition (jmax (0, roundToInt (x * (contentComp->getWidth() - getWidth()))),
                         jmax (0, roundToInt (y * (contentComp->getHeight() - getHeight()))));
}

bool CentreAlignViewport::autoScroll (const int mouseX, const int mouseY, const int activeBorderThickness, const int maximumSpeed)
{
    if (contentComp != 0)
    {
        int dx = 0, dy = 0;
		
        if (horizontalScrollBar.isVisible() || contentComp->getX() < 0 || contentComp->getRight() > getWidth())
        {
            if (mouseX < activeBorderThickness)
                dx = activeBorderThickness - mouseX;
            else if (mouseX >= contentHolder.getWidth() - activeBorderThickness)
                dx = (contentHolder.getWidth() - activeBorderThickness) - mouseX;
			
            if (dx < 0)
                dx = jmax (dx, -maximumSpeed, contentHolder.getWidth() - contentComp->getRight());
            else
                dx = jmin (dx, maximumSpeed, -contentComp->getX());
        }
		
        if (verticalScrollBar.isVisible() || contentComp->getY() < 0 || contentComp->getBottom() > getHeight())
        {
            if (mouseY < activeBorderThickness)
                dy = activeBorderThickness - mouseY;
            else if (mouseY >= contentHolder.getHeight() - activeBorderThickness)
                dy = (contentHolder.getHeight() - activeBorderThickness) - mouseY;
			
            if (dy < 0)
                dy = jmax (dy, -maximumSpeed, contentHolder.getHeight() - contentComp->getBottom());
            else
                dy = jmin (dy, maximumSpeed, -contentComp->getY());
        }
		
        if (dx != 0 || dy != 0)
        {
            contentComp->setTopLeftPosition (contentComp->getX() + dx,
                                             contentComp->getY() + dy);
			
            return true;
        }
    }
	
    return false;
}

void CentreAlignViewport::componentMovedOrResized (Component&, bool, bool)
{
    updateVisibleArea();
}

void CentreAlignViewport::resized()
{
    updateVisibleArea();
}

//==============================================================================
void CentreAlignViewport::updateVisibleArea()
{
    const int scrollbarWidth = getScrollBarThickness();
    const bool canShowAnyBars = getWidth() > scrollbarWidth && getHeight() > scrollbarWidth;
    const bool canShowHBar = showHScrollbar && canShowAnyBars;
    const bool canShowVBar = showVScrollbar && canShowAnyBars;
	
    bool hBarVisible = canShowHBar && ! horizontalScrollBar.autoHides();
    bool vBarVisible = canShowVBar && ! verticalScrollBar.autoHides();
	
    Rectangle<int> contentArea (getLocalBounds());

    if (contentComp != 0 && ! contentArea.contains (contentComp->getBounds()))
    {
        hBarVisible = canShowHBar && (hBarVisible || contentComp->getX() < 0 || contentComp->getRight() > contentArea.getWidth());
        vBarVisible = canShowVBar && (vBarVisible || contentComp->getY() < 0 || contentComp->getBottom() > contentArea.getHeight());
		
        if (vBarVisible)
            contentArea.setWidth (getWidth() - scrollbarWidth);
		
        if (hBarVisible)
            contentArea.setHeight (getHeight() - scrollbarWidth);
		
        if (! contentArea.contains (contentComp->getBounds()))
        {
            hBarVisible = canShowHBar && (hBarVisible || contentComp->getRight() > contentArea.getWidth());
            vBarVisible = canShowVBar && (vBarVisible || contentComp->getBottom() > contentArea.getHeight());
        }
    }
	
    if (vBarVisible)
        contentArea.setWidth (getWidth() - scrollbarWidth);
	
    if (hBarVisible)
        contentArea.setHeight (getHeight() - scrollbarWidth);
	
    contentHolder.setBounds (contentArea);
	
    Rectangle<int> contentBounds;
    if (contentComp != 0)
        contentBounds = contentComp->getBounds();
	
    const Point<int> visibleOrigin (-contentBounds.getPosition());
	
    if (hBarVisible)
    {
        horizontalScrollBar.setBounds (0, contentArea.getHeight(), contentArea.getWidth(), scrollbarWidth);
        horizontalScrollBar.setRangeLimits (0.0, contentBounds.getWidth());
        horizontalScrollBar.setCurrentRange (visibleOrigin.getX(), contentArea.getWidth());
        horizontalScrollBar.setSingleStepSize (singleStepX);
        horizontalScrollBar.cancelPendingUpdate();
    }
	
    if (vBarVisible)
    {
        verticalScrollBar.setBounds (contentArea.getWidth(), 0, scrollbarWidth, contentArea.getHeight());
        verticalScrollBar.setRangeLimits (0.0, contentBounds.getHeight());
        verticalScrollBar.setCurrentRange (visibleOrigin.getY(), contentArea.getHeight());
        verticalScrollBar.setSingleStepSize (singleStepY);
        verticalScrollBar.cancelPendingUpdate();
    }
	
    // Force the visibility *after* setting the ranges to avoid flicker caused by edge conditions in the numbers.
    horizontalScrollBar.setVisible (hBarVisible);
    verticalScrollBar.setVisible (vBarVisible);
	
    const Rectangle<int> visibleArea (visibleOrigin.getX(), visibleOrigin.getY(),
                                      jmin (contentBounds.getWidth() - visibleOrigin.getX(),  contentArea.getWidth()),
                                      jmin (contentBounds.getHeight() - visibleOrigin.getY(), contentArea.getHeight()));
	
    if (lastVisibleArea != visibleArea)
    {
        lastVisibleArea = visibleArea;
        visibleAreaChanged (visibleArea.getX(), visibleArea.getY(), visibleArea.getWidth(), visibleArea.getHeight());
    }
	
    horizontalScrollBar.handleUpdateNowIfNeeded();
    verticalScrollBar.handleUpdateNowIfNeeded();
}

//==============================================================================
void CentreAlignViewport::setSingleStepSizes (const int stepX, const int stepY)
{
    if (singleStepX != stepX || singleStepY != stepY)
    {
        singleStepX = stepX;
        singleStepY = stepY;
        updateVisibleArea();
    }
}

void CentreAlignViewport::setScrollBarsShown (const bool showVerticalScrollbarIfNeeded,
                                   const bool showHorizontalScrollbarIfNeeded)
{
    if (showVScrollbar != showVerticalScrollbarIfNeeded
		|| showHScrollbar != showHorizontalScrollbarIfNeeded)
    {
        showVScrollbar = showVerticalScrollbarIfNeeded;
        showHScrollbar = showHorizontalScrollbarIfNeeded;
        updateVisibleArea();
    }
}

void CentreAlignViewport::setScrollBarThickness (const int thickness)
{
    if (scrollBarThickness != thickness)
    {
        scrollBarThickness = thickness;
        updateVisibleArea();
    }
}

int CentreAlignViewport::getScrollBarThickness() const
{
    return scrollBarThickness > 0 ? scrollBarThickness
	: getLookAndFeel().getDefaultScrollbarWidth();
}

void CentreAlignViewport::setScrollBarButtonVisibility (const bool buttonsVisible)
{
    verticalScrollBar.setButtonVisibility (buttonsVisible);
    horizontalScrollBar.setButtonVisibility (buttonsVisible);
}

void CentreAlignViewport::scrollBarMoved (ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
    const int newRangeStartInt = roundToInt (newRangeStart);
	
    if (scrollBarThatHasMoved == &horizontalScrollBar)
    {
        setViewPosition (newRangeStartInt, getViewPositionY());
    }
    else if (scrollBarThatHasMoved == &verticalScrollBar)
    {
        setViewPosition (getViewPositionX(), newRangeStartInt);
    }
}

void CentreAlignViewport::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& wheel)
{
    if (! useMouseWheelMoveIfNeeded (e, wheel.deltaX, wheel.deltaY))
        Component::mouseWheelMove (e, wheel);
}

bool CentreAlignViewport::useMouseWheelMoveIfNeeded (const MouseEvent& e, float wheelIncrementX, float wheelIncrementY)
{
    if (! (e.mods.isAltDown() || e.mods.isCtrlDown()))
    {
        const bool hasVertBar = verticalScrollBar.isVisible();
        const bool hasHorzBar = horizontalScrollBar.isVisible();
		
        if (hasHorzBar || hasVertBar)
        {
            if (wheelIncrementX != 0)
            {
                wheelIncrementX *= 14.0f * singleStepX;
                wheelIncrementX = (wheelIncrementX < 0) ? jmin (wheelIncrementX, -1.0f)
				: jmax (wheelIncrementX, 1.0f);
            }
			
            if (wheelIncrementY != 0)
            {
                wheelIncrementY *= 14.0f * singleStepY;
                wheelIncrementY = (wheelIncrementY < 0) ? jmin (wheelIncrementY, -1.0f)
				: jmax (wheelIncrementY, 1.0f);
            }
			
            Point<int> pos (getViewPosition());
			
            if (wheelIncrementX != 0 && wheelIncrementY != 0 && hasHorzBar && hasVertBar)
            {
                pos.setX (pos.getX() - roundToInt (wheelIncrementX));
                pos.setY (pos.getY() - roundToInt (wheelIncrementY));
            }
            else if (hasHorzBar && (wheelIncrementX != 0 || e.mods.isShiftDown() || ! hasVertBar))
            {
                if (wheelIncrementX == 0 && ! hasVertBar)
                    wheelIncrementX = wheelIncrementY;
				
                pos.setX (pos.getX() - roundToInt (wheelIncrementX));
            }
            else if (hasVertBar && wheelIncrementY != 0)
            {
                pos.setY (pos.getY() - roundToInt (wheelIncrementY));
            }
			
            if (pos != getViewPosition())
            {
                setViewPosition (pos);
                return true;
            }
        }
    }
	
    return false;
}

bool CentreAlignViewport::keyPressed (const KeyPress& key)
{
    const bool isUpDownKey = key.isKeyCode (KeyPress::upKey)
	|| key.isKeyCode (KeyPress::downKey)
	|| key.isKeyCode (KeyPress::pageUpKey)
	|| key.isKeyCode (KeyPress::pageDownKey)
	|| key.isKeyCode (KeyPress::homeKey)
	|| key.isKeyCode (KeyPress::endKey);
	
    if (verticalScrollBar.isVisible() && isUpDownKey)
        return verticalScrollBar.keyPressed (key);
	
    const bool isLeftRightKey = key.isKeyCode (KeyPress::leftKey)
	|| key.isKeyCode (KeyPress::rightKey);
	
    if (horizontalScrollBar.isVisible() && (isUpDownKey || isLeftRightKey))
        return horizontalScrollBar.keyPressed (key);
	
    return false;
}

