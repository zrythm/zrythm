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

 This file is based around Niall Moody's OSC/UDP library, but was modified to
 be more independent and modularized, using the best practices from JUCE.

 ==============================================================================
*/

BEGIN_JUCE_NAMESPACE

//==============================================================================
OpenSoundController::OpenSoundController()
  : Thread ("OSCThread"),
    port (6789)
{
    sock = new UDPSocket ();
    sock->setPort (port);
}

OpenSoundController::~OpenSoundController()
{
    stopListening ();
    
    deleteAndZero (sock);
}

//==============================================================================
void OpenSoundController::setPort (int newPort)
{
    port = newPort;
    sock->setPort (port);
}

void OpenSoundController::setRootAddress (const String& address)
{
    root = "/" + address + "/";
}

//==============================================================================
void OpenSoundController::startListening()
{
    startThread (5);
}

void OpenSoundController::stopListening()
{
    if (isThreadRunning ())
        stopThread (2000);
}

//==============================================================================
void OpenSoundController::addListener (OpenSoundControllerListener* const listener)
{
    listeners.add (listener);
}

void OpenSoundController::removeListener (OpenSoundControllerListener* const listener)
{
    int index = listeners.indexOf (listener);
    if (index >= 0)
        listeners.remove (index);
}

//==============================================================================
void OpenSoundController::run()
{
    int i;
    OpenSoundBundle *recBundle;
    OpenSoundMessage *recMessage;
    int recursiveSize;
    char *recursiveData;
    long receivedSize = -1;
    char *receivedData = 0;

    sock->bindSocket();

    while (! threadShouldExit ())
    {
        // Listen for OSC packets.
        receivedData = sock->getData (receivedSize);

        if ((receivedSize > -1) && (receivedData))
        {
            // If we've received an OSC message.
            if (OpenSoundMessage::isMessage (receivedData, receivedSize))
            {
                recMessage = new OpenSoundMessage (receivedData, receivedSize);
                handleOSCMessage (recMessage);
                delete recMessage;
            }
            // If we've received an OSC bundle.
            // Note: we don't do anything about timestamps...
            else if (OpenSoundBundle::isBundle (receivedData, receivedSize))
            {
                recBundle = new OpenSoundBundle (receivedData, receivedSize);

                for (i = 0; i < recBundle->getNumMessages(); ++i)
                {
                    recursiveSize = recBundle->getMessage(i)->getSize();
                    recursiveData = recBundle->getMessage(i)->getData();

                    recMessage = new OpenSoundMessage (recursiveData, recursiveSize);
                    handleOSCMessage (recMessage);
                    delete recMessage;
                }
                delete recBundle;
            }
#if JUCE_DEBUG
            else
            {
                printf ("WARNING: Received non-OSC packet: \n");

                for (i = 0; i < receivedSize; ++i)
                    printf ("%c", receivedData[i]);

                printf ("\n");
            }
#endif
        }
    }
}

//==============================================================================
void OpenSoundController::handleOSCMessage (OpenSoundMessage *message)
{
    for (int i = 0; i < listeners.size (); i++)
    {
        OpenSoundControllerListener* listener =
                    (OpenSoundControllerListener*) listeners.getUnchecked (i);
        if (listener->handleOSCMessage (this, message))
            break;
    }
}

//==============================================================================
bool OpenSoundController::isCorrectAddress (const String& address)
{
    return address.indexOf (root) > -1;
}

String OpenSoundController::getPathIndexed (const String& address, const int index)
{
    int old = 1, next = 1, count = 0;

    while ((next = address.indexOf (old, "/")) >= 0)
    {
        if (++count == index)
            return address.substring (old, next);

        old = next + 1;
    }

    if (++count == index)
        return address.substring (old);

    return String();
}

END_JUCE_NAMESPACE