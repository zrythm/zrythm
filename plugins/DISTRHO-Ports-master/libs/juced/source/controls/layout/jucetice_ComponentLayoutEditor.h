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

/* TODO - move to juce_ComponentLayoutManager.h */

#ifndef __JUCETICE_COMPONENTLAYOUTMANAGER_HEADER__
#define __JUCETICE_COMPONENTLAYOUTMANAGER_HEADER__

//=============================================================================
class ChildAlias   :   public Component
{
public:
   ChildAlias (Component* targetChild);
   ~ChildAlias ();

   void resized ();
   void paint (Graphics& g);

   const Component* getTargetChild ();

   void updateFromTarget ();
   void applyToTarget ();

   virtual void userChangedBounds ();
   virtual void userStartedChangingBounds ();
   virtual void userStoppedChangingBounds ();

   bool boundsChangedSinceStart ();

   void mouseEnter (const MouseEvent& e);
   void mouseExit (const MouseEvent& e);
   void mouseDown (const MouseEvent& e);
   void mouseUp (const MouseEvent& e);
   void mouseDrag (const MouseEvent& e);

private:

   CriticalSection bounds;

   ComponentDragger dragger;
   SafePointer<Component> target;
   bool interest;
   bool userAdjusting;
   Rectangle<int> startBounds;
   ResizableBorderComponent* resizer;
};


//=============================================================================
class ComponentLayoutManager   :   public Component
{
public:

   enum ColourIds
   {
      aliasIdleColour,
      aliasHoverColour
   };

   ComponentLayoutManager ();
   ~ComponentLayoutManager ();

   void resized ();
   void paint (Graphics& g);

   void setTargetComponent (Component* target);

   void bindWithTarget ();
   void updateFrames ();

   void enablementChanged ();
   const Component* getTarget ();

private:

   virtual ChildAlias* createAlias (Component* child);

   SafePointer<Component> target;
   OwnedArray<ChildAlias> frames;
};

#endif//_COMPONENTLAYOUTEDITOR_H_ 
