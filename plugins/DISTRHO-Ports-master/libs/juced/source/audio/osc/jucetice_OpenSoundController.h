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

#ifndef __JUCETICE_OPENSOUNDCONTROLLER_HEADER__
#define __JUCETICE_OPENSOUNDCONTROLLER_HEADER__

#include "jucetice_UDPSocket.h"
#include "jucetice_OpenSoundBundle.h"
#include "jucetice_OpenSoundMessage.h"
#include "jucetice_OpenSoundTimeTag.h"

class OpenSoundController;


//================================================================================
/**
    Listener to an OSC controller
 */
class OpenSoundControllerListener
{
public:

    /** Public destructor */
    virtual ~OpenSoundControllerListener () {}

    /** You should write your own message handler */
    virtual bool handleOSCMessage (OpenSoundController* controller,
                                   OpenSoundMessage *message) = 0;
};


//================================================================================
/**
    Singleton class handling OSC input.

    Runs a separate thread listening on the relevant port for OSC messages.
    When a message is received, the class calls the listeners

    @todo Handle timestamps for bundles.
 */
class OpenSoundController : public Thread
{
public:

    //================================================================================
    /** Constructor */
    OpenSoundController();

    /** Destructor */
    ~OpenSoundController();

    //================================================================================
    /** Starts listening for OSC messages on the specified port. */
    void startListening();

    /** Stops listening for OSC messages. */
    void stopListening();

    /** The thread loop */
    void run();

    //================================================================================
    /** Called to set the port to listen on. */
    void setPort (const int newPort);

    /** Returns the actual used socket port */    
    int getPort () const                                        { return port; }

    //================================================================================
    /** Just checks if the first section of the address is one of the desired */
    void setRootAddress (const String& address);

    //================================================================================
    /** Add a new OSC listener */
    void addListener (OpenSoundControllerListener* const listener);

    /** Remove a previously added OSC listener */
    void removeListener (OpenSoundControllerListener* const listener);

    //================================================================================
    /** Just checks if the first section of the address is one of the desired */
    bool isCorrectAddress (const String& address);

    /** Returns the indexed address in an OSC address.

        i.e. we'd receive address = /xxx/yyy/zzz ;
        this method returns zzz if index = 2
    */
    String getPathIndexed (const String& address, const int index);

protected:

    //================================================================================
    /** Passes on an OSC message to all the relevant nodes. */
    void handleOSCMessage (OpenSoundMessage *message);

    UDPSocket* sock;
    int port;
    String root;

    Array<void*> listeners;
};

#endif
