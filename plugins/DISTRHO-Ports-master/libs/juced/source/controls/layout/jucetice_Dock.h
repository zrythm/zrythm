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

#ifndef __JUCETICE_DOCK_HEADER__
#define __JUCETICE_DOCK_HEADER__

//==============================================================================
/**
    Dockable item.

    For example, if you have a component that is part of the Dock, you would
    subclass your object with the Dockable also, so that component added to the
    Dock will be draggable onto another position in it.

    You can choose to make this draggable/not draggable by calling/not calling
    its startDocking and continueDocking in your mouseDown/mouseDrag.

    E.g.
    @code

    @endcode
*/
class Dockable
{
public:

    //==============================================================================
    /** Destruct a Dockable */
    virtual ~Dockable ();


    //==============================================================================
    /** In your subclass you have to setup a known name that will be passed on
        the drag container when the item is moved or dragged

        @return     string that identifies this Dockable
    */
    virtual String getDragDescription () = 0;

    /** In your subclass you should return the component that belongs to this
        Dockable.

        Typically you'll return "this". eventually you can make a subcomponent being
        dragged around but remember that is the higher component that will be added
        to the Dock.
    */
    virtual Component* getDragComponent () = 0;

    /** In your subclass you have to return the draggable image that will be shown
        when the item is dragged around.

        You shouldn't keep a pointer to the image as this would be deleted
        internally by the DragAndDropContainer, that will be our Dock class.

        @return     image to display when dragged around, 0 if you do not want
                    any image to be shown when dragged
    */
    virtual Image getDragImage () = 0;

    //==============================================================================
    /**
        Start the drag onto another Dockable.

        You should normally call this from your mouseDown method if you want to
        make your component draggable over another component place in the Dock.

        @param e            mouse event (that comes from mouseDown)
    */
    virtual void startDocking (const MouseEvent& e);

    /**
        Handle dragging onto another Dockable.

        You should normally call this from your mouseDrag method if you want to
        make your component draggable over another component place in the Dock.

        @param e            mouse event (that comes from mouseDrag)
    */
    virtual void continueDocking (const MouseEvent& e);

protected:

    //==============================================================================
    /** Protected constructor */
    Dockable ();

    bool isDragging;
};


//==============================================================================
/**
    Dock panel which containes Dockable components.

    When you construct this objectFor example, if you have a component that is
    part of the Dock, you would subclass your object with the Dockable also,
    so that component added to the Dock will be draggable over another position
    on the Dock.

    You can choose to make this draggable/not draggable by calling/not calling
    its startDocking and continueDocking in your mouseDown/mouseDrag.

    E.g.
    @code

    @endcode
*/
class DockPanel
{
public:

    //==============================================================================
    /** Construct a DockPanel class */
    DockPanel ();

    //==============================================================================
    /** This lets you add a set of components to the panel

        @param holderComponent          the component in which the subcomponents
                                        will be added, this could be a Dock, or your
                                        own component
        @param componentsToPlace        array of component pointers to be added in
                                        dock panel
        @param numOfComponents          how many components we have to add
        @param isStackedVertical        if the components are stacked vertical
        @param parentDockPanel          pointer to the parent DockPanel, if this is
                                        placed inside another layout
        @param parentDockIndex          itemLayout index of the parent in which this
                                        DockPanel is placed.
    */
    void placeComponents (Component* holderComponent,
                          Component** componentsToPlace,
                          const int numOfComponents,
                          const bool isStackedVertical,
                          DockPanel* parentDockPanel = 0,
                          const int parentDockIndex = 0);

    //==============================================================================
    /**
        Update the current itemLayout for the components being part
        of this DockPanel.

        Tipically you would call this in your parent component resized method
    */
    void resizedTo (const int x, const int y, const int width, const int height);

    //==============================================================================
    /** Swap a component with one of another panel

        @param componentSource          index of the component source to be swap
        @param panelDestination         panel from which pick the destination
                                        component to be swapped with source
        @param componentDestination     index of the component destination
     */
    void swapComponent (int componentSource,
                        DockPanel* panelDestination,
                        int componentDestination);

    /** Find if a component belongs to this panel

        @param componentToSearch        component to find
        @param componentIndex           this will be filled with the index of the
                                        component if found, < 0 if not

        @return                         true if the component is in this panel,
                                        false otherwise
    */
    bool findComponent (Component* const componentToSearch,
                        int& componentIndex);

protected:

    Component* holder;
    bool isVertical;

    DockPanel* parentDock;
    int parentIndex;

    StretchableLayoutManager layout;
    Array<void*> components;
};



//==============================================================================
/**
    This is the parent Component container for DockPanel components.

    E.g.
    @code

    @endcode
*/
class Dock : public Component,
             public DragAndDropTarget
{
public:


    //==============================================================================
    /** Construct a Dock that will containe DockPanels */
    Dock ();

    /** Destructor */
    ~Dock ();

    //==============================================================================
    /** Add a new DockPanel to our Dock container

        @param components           array of Component's pointer to be added. in the
                                    array you could keep some items null to let other
                                    panels be fitted in that space
        @param numComponents        number of components in array
        @param isVertical           if the panel is vertical or horizontal
        @param parentPanelIndex     index of the panel in which construct this panel.
                                    if this is < 0 then no parents will be linked
        @param parentPanelOffset    offset in the panel components array where fit
                                    this new panel. typically the value at this
                                    index in the components array should be null.
                                    if this is < 0 then no parents will be linked
    */
    int addDockPanel (Component** components,
                      const int numComponents,
                      const bool isVertical,
                      const int parentPanelIndex = -1,
                      const int parentPanelOffset = -1);

    //==============================================================================
    /** Derived from DragAndDropTarget */
    bool isInterestedInDragSource (const String& sourceDescription);

    /** Derived from DragAndDropTarget */
    void itemDragEnter (const SourceDetails& dragSourceDetails);

    /** Derived from DragAndDropTarget */
    void itemDragMove (const SourceDetails& dragSourceDetails);

    /** Derived from DragAndDropTarget */
    void itemDragExit (const SourceDetails& dragSourceDetails);

    /** Derived from DragAndDropTarget */
    void itemDropped (const SourceDetails& dragSourceDetails);

    //==============================================================================
    /** @internal */
    void resized();
    /** @internal */
    void paintOverChildren (Graphics& g);

    //==============================================================================
    juce_UseDebuggingNewOperator

protected:

    bool findComponent (Component* const componentToSearch,
                        int& dockPanelIndex,
                        int& dockPanelSubIndex);

    bool somethingIsBeingDraggedOver;
    OwnedArray<DockPanel> panels;
};

#endif // __JUCETICE_JOSTDOCK_HEADER__
