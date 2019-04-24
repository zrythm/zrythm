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

/* TODO - move to juce_ComponentLayoutManager.cpp */

BEGIN_JUCE_NAMESPACE

//==============================================================================
ChildAlias::ChildAlias (Component* targetChild)
:   target (targetChild)
{
   resizer = new ResizableBorderComponent (this,0);
   addAndMakeVisible (resizer);
   resizer->addMouseListener (this,false);

   interest = false;
   userAdjusting = false;

   updateFromTarget ();
   setRepaintsOnMouseActivity (true);
}

ChildAlias::~ChildAlias ()
{
   delete resizer;
}

void ChildAlias::resized ()
{
   resizer->setBounds (0,0,getWidth(),getHeight());

   if (resizer->isMouseButtonDown ())
   {
      applyToTarget ();
   }
}

void ChildAlias::paint (Graphics& g)
{
   Colour c;
   if (interest)
      c = findColour (ComponentLayoutManager::aliasHoverColour, true);
   else
      c = findColour (ComponentLayoutManager::aliasIdleColour, true);

   g.setColour (c.withMultipliedAlpha (0.3f));
   g.fillAll ();
   g.setColour (c);
   g.drawRect (0, 0, getWidth(), getHeight(), 3);
}

const Component* ChildAlias::getTargetChild ()
{
   return target.getComponent ();
}

void ChildAlias::updateFromTarget ()
{
   if (target != 0)
   {
      setBounds ( target.getComponent ()->getBounds () );
   }
}

void ChildAlias::applyToTarget ()
{
   if (target != 0)
   {
      Component* c = (Component*) target.getComponent ();
      c->setBounds (getBounds ());
      userChangedBounds ();
   }
}

void ChildAlias::userChangedBounds ()
{
}

void ChildAlias::userStartedChangingBounds ()
{
}

void ChildAlias::userStoppedChangingBounds ()
{
}

bool ChildAlias::boundsChangedSinceStart ()
{
   return startBounds != getBounds ();
}


void ChildAlias::mouseDown (const MouseEvent& e)
{
   toFront (true);
   if (e.eventComponent == resizer)
   {
   }
   else
   {
      dragger.startDraggingComponent (this, e);
   }
   userAdjusting = true;
   startBounds = getBounds ();
   userStartedChangingBounds ();
}

void ChildAlias::mouseUp (const MouseEvent& e)
{
   if (e.eventComponent == resizer)
   {
   }
   else
   {
   }
   if (userAdjusting) userStoppedChangingBounds ();
   userAdjusting = false;
}

void ChildAlias::mouseDrag (const MouseEvent& e)
{
   if (e.eventComponent == resizer)
   {
   }
   else
   {
      if (!e.mouseWasClicked ())
      {
         dragger.dragComponent (this, e, 0);
         applyToTarget ();
      }
   }
}

void ChildAlias::mouseEnter (const MouseEvent& e)
{
   interest = true;
   repaint ();
}

void ChildAlias::mouseExit (const MouseEvent& e)
{
   interest = false;
   repaint ();
}

//=============================================================================
ComponentLayoutManager::ComponentLayoutManager ()
:   target (0)
{
   setColour (ComponentLayoutManager::aliasIdleColour,Colours::lightgrey.withAlpha(0.2f));
   setColour (ComponentLayoutManager::aliasHoverColour,Colours::white.withAlpha(0.5f));
   
   setInterceptsMouseClicks (false, true);
}

ComponentLayoutManager::~ComponentLayoutManager ()
{
   if (target) { deleteAndZero (target); }
}

void ComponentLayoutManager::resized ()
{
   for (int i=0; i<frames.size(); i++)
   {
      frames.getUnchecked(i)->updateFromTarget ();
   }
}

void ComponentLayoutManager::paint (Graphics& g)
{
}

void ComponentLayoutManager::setTargetComponent (Component* targetComp)
{
   jassert (targetComp);
   jassert (targetComp->getParentComponent() == getParentComponent());

   if (target)
   {
      if (target.getComponent() == targetComp) return;
      deleteAndZero (target);
   }

   target = targetComp;
   bindWithTarget ();
}

void ComponentLayoutManager::bindWithTarget ()
{
   if (target != 0)
   {
      Component* t = (Component*) target.getComponent ();
      Component* p = t->getParentComponent ();

      p->addAndMakeVisible (this);
      setBounds (t->getBounds ());

      updateFrames ();
   }
}

void ComponentLayoutManager::updateFrames ()
{
   frames.clear ();

   if (target != 0)
   {
      Component* t = (Component*) target.getComponent ();

      int n = t->getNumChildComponents ();
      for (int i=0; i<n; i++)
      {
         Component* c = t->getChildComponent (i);
         if (c)
         {
            ChildAlias* alias = createAlias (c);
            if (alias)
            {
               frames.add (alias);
               addAndMakeVisible (alias);
            }
         }
      }
   }
}

void ComponentLayoutManager::enablementChanged ()
{
   if (isEnabled ())
   {
      setVisible (true);
   }
   else
   {
      setVisible (false);
   }
}

const Component* ComponentLayoutManager::getTarget ()
{
   if (target) return target.getComponent ();
   return 0;
}

ChildAlias* ComponentLayoutManager::createAlias (Component* child)
{
   return new ChildAlias (child);
} 

END_JUCE_NAMESPACE
