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
MidiAutomatable::MidiAutomatable()
  : controllerNumber (-1),
#if 0
    transfer (MidiAutomatable::Linear),
#endif
    midiAutomatorManager (0)
{
#if 0
    function = MakeDelegate (this, &MidiAutomatable::transferFunctionLinear);
#endif
}

MidiAutomatable::~MidiAutomatable()
{
    if (midiAutomatorManager)
        midiAutomatorManager->removeMidiAutomatable (this); 
}

void MidiAutomatable::activateLearning ()
{
    jassert (midiAutomatorManager != 0); // if you fallback here then you don't have registered your
                                         // midi automatable object to the manager before actually starting
                                         // the midi learn feature

    if (midiAutomatorManager)
        midiAutomatorManager->setActiveLearner (this);
}

void MidiAutomatable::setControllerNumber (const int control)
{
    if (controllerNumber != control)
    {
        controllerNumber = control;

        if (midiAutomatorManager)
            midiAutomatorManager->registerMidiAutomatable (this);    
    }
}

#if 0
void MidiAutomatable::setTransferFunction (MidiTransferFunction newFunction)
{
    switch (newFunction)
    {
    default:
    case MidiAutomatable::Linear:
        function = MakeDelegate (this, &MidiAutomatable::transferFunctionLinear);
        break;
    case MidiAutomatable::InvertedLinear:
        function = MakeDelegate (this, &MidiAutomatable::transferFunctionLinear);
        break;
    }

    transfer = newFunction;
}
#endif

void MidiAutomatable::setMidiAutomatorManager (MidiAutomatorManager* newManager)
{
    midiAutomatorManager = newManager;
}

#if 0
float MidiAutomatable::transferFunctionLinear (int ccValue)
{
    return ccValue * float_MidiScaler;
}

float MidiAutomatable::transferFunctionInvertedLinear (int ccValue)
{
    return 1.0f - ccValue * float_MidiScaler;
}
#endif

void MidiAutomatable::handleMidiPopupMenu (const MouseEvent& e)
{
    PopupMenu menu, ccSubMenu;
    
    const int controllerNumber = getControllerNumber ();

    for (int i = 0; i < 128; i++)
    {
         String name (MidiMessage::getControllerName (i));

         ccSubMenu.addItem (i + 1000,
                            "CC# " + String(i) + " " + name,
                            true,
                            controllerNumber == i);
    }
    
    if (controllerNumber != -1)
        menu.addItem (-1, "Assigned to CC# " + String (controllerNumber), false);
    else
        menu.addItem (-1, "Not assigned", false);
    menu.addSeparator ();

    menu.addItem (1, "Midi Learn");
    menu.addSubMenu ("Set CC", ccSubMenu);
    menu.addItem (2, "Reset CC", controllerNumber != -1);
    
    int result = menu.show ();
    switch (result)
    {
    case 1:
        activateLearning ();
        break;
    case 2:
        setControllerNumber (-1);
        break;
    default:
        if (result >= 1000 && result < 1128)
            setControllerNumber (result - 1000);
        break;
    }
}

//==============================================================================
MidiAutomatorManager::MidiAutomatorManager ()
    : activeLearner (0)
{
    for (int i = 0; i < 128; i++)
        controllers.add (new Array<void*>);
}

MidiAutomatorManager::~MidiAutomatorManager ()
{
    for (int i = 0; i < 128; i++)
        delete controllers.getUnchecked (i);
}

//==============================================================================
void MidiAutomatorManager::registerMidiAutomatable (MidiAutomatable* object)
{
    object->setMidiAutomatorManager (this);

    for (int i = 0; i < 128; i++)
    {
        Array<void*>* array = controllers.getUnchecked (i);

        if (array->contains (object))
        {
            array->remove (array->indexOf (object));
            break;
        }
    }
    
    if (object->getControllerNumber () != -1)
    {
        Array<void*>* array = controllers.getUnchecked (object->getControllerNumber ());
        
        array->add (object);
    }
}

//==============================================================================
void MidiAutomatorManager::removeMidiAutomatable (MidiAutomatable* object)
{
    if (activeLearner == object)
        activeLearner = 0;

    for (int i = 0; i < 128; i++)
    {
        Array<void*>* array = controllers.getUnchecked (i);
        
        if (array->contains (object))
        {
            array->remove (array->indexOf (object));
            break;
        }
    }
}

//==============================================================================
void MidiAutomatorManager::clearMidiAutomatableFromCC (const int ccNumber)
{
    jassert (ccNumber >= 0 && ccNumber < 128);

    Array<void*>* array = controllers.getUnchecked (ccNumber);
    
    array->clear();
}
    
//==============================================================================
void MidiAutomatorManager::setActiveLearner (MidiAutomatable* object)
{
    activeLearner = object;
}

//==============================================================================
bool MidiAutomatorManager::handleMidiMessage (const MidiMessage& message)
{
    bool messageWasHandled = false;

    if (message.isController ())
    {
        if (activeLearner != 0)
        {
            activeLearner->setControllerNumber (message.getControllerNumber ());
            activeLearner = 0;
        }
        else
        {
            Array<void*>* array = controllers.getUnchecked (message.getControllerNumber ());
            
            for (int i = 0; i < array->size (); i++)
            {
                MidiAutomatable* learnObject = (MidiAutomatable*) array->getUnchecked (i);
                
                messageWasHandled |= learnObject->handleMidiMessage (message);
            }
        }
    }
    
    return messageWasHandled;
}

//==============================================================================
bool MidiAutomatorManager::handleMidiMessageBuffer (MidiBuffer& buffer)
{
    int samplePosition;
    MidiMessage message (0xf4);
    MidiBuffer::Iterator it (buffer);

    bool messageWasHandled = false;

    while (it.getNextEvent (message, samplePosition))
    {
        messageWasHandled |= handleMidiMessage (message);
    }
    
    return messageWasHandled;
}

END_JUCE_NAMESPACE

