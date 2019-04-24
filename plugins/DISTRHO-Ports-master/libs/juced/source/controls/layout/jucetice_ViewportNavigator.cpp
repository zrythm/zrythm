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
ViewportNavigator::ViewportNavigator (JuceticeViewport* const viewportToWatch)
    : watchedViewport (0),
      componentSnapshot (0),
      viewPositionX (0),
      viewPositionY (0),
      viewWidth (0),
      viewHeight (0),
      dragViewPositionX (-1),
      dragViewPositionY (-1)
{
    setOpaque (true);
    setWantsKeyboardFocus (false);

    setViewedViewport (viewportToWatch);
}

ViewportNavigator::~ViewportNavigator ()
{
    setViewedViewport (0);

    stopTimer ();
}

//==============================================================================
void ViewportNavigator::resized ()
{
    updateVisibleArea ();
}

void ViewportNavigator::paint (Graphics& g)
{
    if (componentSnapshot.isValid())
    {
        g.drawImage (componentSnapshot,
                     0, 0, getWidth (), getHeight (),
                     0, 0, componentSnapshot.getWidth (), componentSnapshot.getHeight (),
                     false);
    }
    else
    {
        g.fillAll (Colours::black);
    }
    
    if (watchedViewport)
    {
        g.setColour (Colours::red.withAlpha (0.55f));
        g.drawRect (viewPositionX * getWidth (),
                    viewPositionY * getHeight (),
                    viewWidth * getWidth (),
                    viewHeight * getHeight (),
                    2.0f);
    }
}

//==============================================================================
void ViewportNavigator::mouseDown (const MouseEvent& e)
{
    if (e.mods.isLeftButtonDown ())
    {
        if (isPointInsideCurrentViewArea (e.x, e.y))
        {
            dragViewPositionX = viewPositionX; 
            dragViewPositionY = viewPositionY;
            
            setMouseCursor (MouseCursor::DraggingHandCursor);
        }
    }
}

void ViewportNavigator::mouseMove (const MouseEvent& e)
{
    if (isPointInsideCurrentViewArea (e.x, e.y))
    {
        setMouseCursor (MouseCursor::DraggingHandCursor);
    }
    else
    {
        setMouseCursor (MouseCursor::NormalCursor);
    }
}

void ViewportNavigator::mouseDrag (const MouseEvent& e)
{
    if (e.mods.isLeftButtonDown ()
        && dragViewPositionX >= 0 && dragViewPositionX >= 0 )
    {
        if (watchedViewport)
        {
            Component* comp = watchedViewport->getViewedComponent ();
            if (comp)
            {
                float startX = dragViewPositionX + e.getDistanceFromDragStartX () / (float) getWidth ();
                float startY = dragViewPositionY + e.getDistanceFromDragStartY () / (float) getHeight ();

                startX = jmax (0.0f, jmin (1.0f - viewWidth, startX));
                startY = jmax (0.0f, jmin (1.0f - viewHeight, startY));

                watchedViewport->setViewPosition (roundFloatToInt (startX * comp->getWidth ()),
                                                  roundFloatToInt (startY * comp->getHeight ()));
            }
        }
    }
}

void ViewportNavigator::mouseUp (const MouseEvent& e)
{
    dragViewPositionX = -1;
    dragViewPositionY = -1;

    if (isPointInsideCurrentViewArea (e.x, e.y))
    {
        setMouseCursor (MouseCursor::DraggingHandCursor);
    }
    else
    {
        setMouseCursor (MouseCursor::NormalCursor);
    }
}

//==============================================================================
void ViewportNavigator::timerCallback ()
{
    updateVisibleArea (false);
}

//==============================================================================
JuceticeViewport* ViewportNavigator::getViewedViewport ()
{
    return watchedViewport;
}

void ViewportNavigator::setViewedViewport (JuceticeViewport* const viewportToWatch)
{
    bool needsUpdate = false;

    if (watchedViewport)
    {
        Component* comp = watchedViewport->getViewedComponent ();
        if (comp)
            comp->removeComponentListener (this);

        watchedViewport->removeComponentListener (this);
        watchedViewport->removeListener (this);
        
        if (! viewportToWatch)
            needsUpdate = true;
    }

    watchedViewport = viewportToWatch;

    if (watchedViewport)
    {
        Component* comp = watchedViewport->getViewedComponent ();
        if (comp != 0)
            comp->addComponentListener (this);

        watchedViewport->addListener (this);
        watchedViewport->addComponentListener (this);
        
        needsUpdate = true;
    }

    if (needsUpdate)
    {
        updateVisibleArea ();
    }
}

//==============================================================================
void ViewportNavigator::componentMovedOrResized (Component& component, bool wasMoved, bool wasResized)
{
    bool needsRepaint = false;
    
    Component* componentChanged = &component;

    if (wasResized)
    {
        if (watchedViewport == componentChanged)
        {
            updateViewPosition ();
            needsRepaint = true;
        }

        if (watchedViewport
            && watchedViewport->getViewedComponent () == componentChanged)
        {
            updateSnapshot ();
            updateViewPosition ();
            needsRepaint = true;
        }
    }

    if (needsRepaint)
    {            
        repaint ();
    }
}

void ViewportNavigator::viewedComponentChanged (JuceticeViewport* const viewportThatChanged,
                                                Component* const newViewedComponent)
{
    updateVisibleArea ();
}

void ViewportNavigator::visibleAreaChanged (JuceticeViewport* const viewportThatChanged,
                                            int visibleX, int visibleY,
                                            int visibleW, int visibleH)
{
    updateViewPosition ();
    repaint ();
}

//==============================================================================
void ViewportNavigator::updateVisibleArea (const bool asyncronously)
{
    if (asyncronously)
    {
        startTimer (500);
    }
    else
    {
        stopTimer ();
        
        updateSnapshot ();
        updateViewPosition ();

        repaint ();
    }
}

void ViewportNavigator::updateSnapshot ()
{
    if (watchedViewport
        && getWidth () > 0 && getHeight () > 0)
    {
        Component* comp = watchedViewport->getViewedComponent ();
        if (comp)
        {
            Image oldSnapshot = componentSnapshot;

            Rectangle<int> areaToGrab;
            areaToGrab.setSize (comp->getWidth (), comp->getHeight ());

            Image componentSnapshotCopy = comp->createComponentSnapshot (areaToGrab, false);
            componentSnapshot = componentSnapshotCopy.rescaled (getWidth (), getHeight ());

        }
    }
}

void ViewportNavigator::updateViewPosition ()
{
    if (watchedViewport)
    {
        Component* comp = watchedViewport->getViewedComponent ();
        if (comp)
        {
            viewPositionX = watchedViewport->getViewPositionX () / (float) comp->getWidth ();
            viewPositionY = watchedViewport->getViewPositionY () / (float) comp->getHeight ();
            viewWidth = watchedViewport->getViewWidth () / (float) comp->getWidth ();
            viewHeight = watchedViewport->getViewHeight () / (float) comp->getHeight ();
        }
    }
}

bool ViewportNavigator::isPointInsideCurrentViewArea (const float x, const float y) const
{
    float viewX = viewPositionX * getWidth ();
    float viewY = viewPositionY * getHeight ();
    float viewW = viewWidth * getWidth ();
    float viewH = viewHeight * getHeight ();

    return (x >= viewX && x <= (viewX + viewW)
            && y >= viewY && y <= (viewY + viewH));
}

END_JUCE_NAMESPACE
