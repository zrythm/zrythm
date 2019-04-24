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

#ifndef __JUCETICE_VIEWPORT_HEADER__
#define __JUCETICE_VIEWPORT_HEADER__

class JuceticeViewport;

//==============================================================================
/**
	Gets informed about changes to a viewport area or viewed component changes.
	Be sure to deregister listeners before you delete them!
 
	@see JuceticeViewport::addListener, JuceticeViewport::removeListener
*/
class JUCE_API  ViewportListener
{
public:
	
    /** Destructor. */
    virtual ~ViewportListener() { }
	
    /** Called when the viewport viewed component is changed
	 */
    virtual void viewedComponentChanged (JuceticeViewport* const viewportThatChanged,
                                         Component* const newViewedComponent);
	
    /** Called when the viewport visible area is changed
	 */
    virtual void visibleAreaChanged (JuceticeViewport* const viewportThatChanged,
                                     int visibleX, int visibleY,
                                     int visibleW, int visibleH);
};


//==============================================================================
/**
    Custom Viewport with listeners.
 
*/
class JuceticeViewport : public Viewport
{
public:

    //==============================================================================
	/** Constructor */
    JuceticeViewport (const String& componentName = String());
	
    /** Destruct the Viewport */
    ~JuceticeViewport ();

	//==============================================================================
    /** @internal */
    void visibleAreaChanged (const Rectangle<int>& newVisibleArea);
	
    /** @internal */
    void viewedComponentChanged (Component* newComponent);
	
	//==============================================================================
    /**
		Adds a listener to be told about changes to the viewport.
	 
		Viewport listeners get called when the visible area changes or the internal
		viewed component changes - see the ViewportListener class for more details.
	 
		@param newListener  the listener to register - if this is already registered, it
							will be ignored.
		@see ViewportListener, removeListener
	*/
    void addListener (ViewportListener* const newListener) throw();
	
    /** Removes a viewport listener.
	 
		@see addListener
	*/
    void removeListener (ViewportListener* const listenerToRemove) throw();
	
    /**
		Notify the listeners that the current viewed component has changed.
	 
		Useful when the viewed component changes some way, and we need to 
		notify all the listeners to update themselves, and is much more
		easier to use findParentComponentOfClass() and get the parent Viewport
		then call this function (we don't need to remember pointers).
	*/
    void notifyComponentChanged ();
	
    //==============================================================================
	juce_UseDebuggingNewOperator
    

private:
	
	Array<void*> viewportListeners;


};

#endif // __JUCETICE_VIEWPORT_HEADER__
