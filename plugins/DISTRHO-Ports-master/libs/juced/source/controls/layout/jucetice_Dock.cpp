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
Dockable::Dockable ()
    : isDragging (false)
{
}

Dockable::~Dockable ()
{
}

void Dockable::startDocking (const MouseEvent& /* e */)
{
    isDragging = false;
}

void Dockable::continueDocking (const MouseEvent& e)
{
    if (! (e.mouseWasClicked() || isDragging))
    {
        const String dragDescription (getDragDescription());
        if (dragDescription.isNotEmpty())
        {
            isDragging = true;

            Component* const dragSourceComponent = getDragComponent();

            DragAndDropContainer* const dragContainer
                = DragAndDropContainer::findParentDragContainerFor (dragSourceComponent);

            if (dragContainer != 0)
            {
                dragContainer->startDragging (dragDescription,
                                              dragSourceComponent,
                                              getDragImage ());
            }
            else
            {
                // to be able to do a drag-and-drop operation, the component needs to
                // be inside a component which is also a DragAndDropContainer.
                jassertfalse;
            }
        }
    }
}


//==============================================================================
DockPanel::DockPanel ()
    : holder (0),
      isVertical (false),
      parentDock (0),
      parentIndex (0)
{
}

void DockPanel::placeComponents (Component* holderComponent,
                                 Component** componentsToPlace,
                                 const int numOfComponents,
                                 const bool isStackedVertical,
                                 DockPanel* parentDockPanel,
                                 const int parentDockIndex)
{
    jassert (numOfComponents > 0);

    holder = holderComponent;
    isVertical = isStackedVertical;
    parentDock = parentDockPanel;
    parentIndex = parentDockIndex;

    for (int i = 0, x = 0; i < (2 * numOfComponents - 1); i++)
    {
        if (i % 2 == 0)
        {
            layout.setItemLayout (i, -0.1, -1.0, - (1.0 / numOfComponents));

            if (componentsToPlace [x])
                holder->addAndMakeVisible (componentsToPlace [x]);
            components.add (componentsToPlace [x]);
            x++;
        }
        else
        {
            layout.setItemLayout (i, 3, 3, 3);

            StretchableLayoutResizerBar* resizer =
                                new StretchableLayoutResizerBar (&layout, i, !isVertical);
            holder->addAndMakeVisible (resizer);
            components.add (resizer);
        }
    }
}

void DockPanel::resizedTo (const int x, const int y, const int width, const int height)
{
    if (components.size() > 0)
    {
        int currentX = x,
            currentY = y,
            currentWidth = width,
            currentHeight = height;

        DockPanel* currentPanel = this;
        DockPanel* currentParent = parentDock;

        while (currentParent != 0)
        {
            int index = currentPanel->parentIndex * 2;

            if (currentParent->isVertical)
            {
                currentY = currentParent->layout.getItemCurrentPosition (index);
                currentHeight = currentParent->layout.getItemCurrentAbsoluteSize (index);
            }
            else
            {
                currentX = currentParent->layout.getItemCurrentPosition (index);
                currentWidth = currentParent->layout.getItemCurrentAbsoluteSize (index);
            }

            currentPanel = currentParent;
            currentParent = currentPanel->parentDock;
        }

        layout.layOutComponents ((Component**) &(components.getReference (0)),
                                 components.size(),
                                 currentX, currentY, currentWidth, currentHeight,
                                 isVertical,
                                 true);
    }
}

void DockPanel::swapComponent (int componentSource,
                               DockPanel* panelDestintation,
                               int componentDestination)
{
    Component* source = (Component*) components [componentSource];
    Component* dest = (Component*) panelDestintation->components [componentDestination];

//  if (dest != 0 && source != 0) // @XXX - actually this is not needed
    {
        panelDestintation->components.set (componentDestination, source);
        components.set (componentSource, dest);
    }
}

bool DockPanel::findComponent (Component* const componentToSearch,
                               int& componentIndex)
{
    componentIndex = components.indexOf (componentToSearch);

    return (componentIndex >= 0
            && componentIndex < components.size()
            && componentIndex % 2 == 0);
}


//==============================================================================
Dock::Dock ()
    : somethingIsBeingDraggedOver (false)
{
    setOpaque (false);
}

Dock::~Dock()
{
    deleteAllChildren();
}

int Dock::addDockPanel (Component** components,
                        const int numComponents,
                        const bool isVertical,
                        const int parentPanelIndex,
                        const int parentPanelOffset)
{
    int parentDockIndex = 0;
    DockPanel* parentDock = 0;

    if (parentPanelIndex >= 0
        && parentPanelIndex < panels.size()
        && parentPanelOffset >= 0)
    {
        parentDock = panels [parentPanelIndex];
        parentDockIndex = parentPanelOffset;
    }

    DockPanel* dock = new DockPanel ();

    dock->placeComponents (this,
                           components,
                           numComponents,
                           isVertical,
                           parentDock,
                           parentDockIndex);

    panels.add (dock);

    return panels.size() - 1;
}

void Dock::resized()
{
    for (int i = 0; i < panels.size(); i++)
    {
        panels.getUnchecked (i)->resizedTo (0, 0, getWidth(), getHeight());
    }
}

void Dock::paintOverChildren (Graphics& g) {

    if (somethingIsBeingDraggedOver)
    {
        g.saveState();
        g.setColour (Colours::red);
        g.drawRect (0, 0, getWidth(), getHeight(), 2);
        g.restoreState();
    }
}

bool Dock::isInterestedInDragSource (const String& sourceDescription)
{
    // normally you'd check the sourceDescription value to see if it's the
    // sort of object that you're interested in before returning true, but for
    // the demo, we'll say yes to anything..
    return true;
}

void Dock::itemDragEnter (const SourceDetails& dragSourceDetails)
{
    somethingIsBeingDraggedOver = true;
    repaint();
}

void Dock::itemDragMove (const SourceDetails& dragSourceDetails)
{
}

void Dock::itemDragExit (const SourceDetails& dragSourceDetails)
{
    somethingIsBeingDraggedOver = false;
    repaint();
}

void Dock::itemDropped (const SourceDetails& dragSourceDetails)
{
    int sourcePanelIndex = -1, sourcePanelSubIndex = -1,
        targetPanelIndex = -1, targetPanelSubIndex = -1;

	Point<int> p (dragSourceDetails.localPosition);
    Component* targetComponent = getComponentAt (p);

    if (findComponent (dragSourceDetails.sourceComponent, sourcePanelIndex, sourcePanelSubIndex)
        && findComponent (targetComponent, targetPanelIndex, targetPanelSubIndex))
    {
        panels.getUnchecked (sourcePanelIndex)->swapComponent (sourcePanelSubIndex,
                                                               panels.getUnchecked (targetPanelIndex),
                                                               targetPanelSubIndex);

        resized();
    }

    somethingIsBeingDraggedOver = false;
    repaint();
}

bool Dock::findComponent (Component* const componentToSearch,
                          int& dockPanelIndex,
                          int& dockPanelSubIndex)
{
    for (dockPanelIndex = 0; dockPanelIndex < panels.size(); dockPanelIndex++)
    {
        if (panels.getUnchecked (dockPanelIndex)->findComponent (componentToSearch,
                                                                 dockPanelSubIndex))
        {
            return true;
        }
    }

    return false;
}

END_JUCE_NAMESPACE
