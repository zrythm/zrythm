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

#ifndef __JUCETICE_EXTERNALTRANSPORT_HEADER__
#define __JUCETICE_EXTERNALTRANSPORT_HEADER__

//==============================================================================
/**
    External transport class.

    This class is a wrapper class for external sync, especially useful to let
    your application grab external synchornization (ie JACK) and control the
    master tempo, time signature and frame position of the main playback.
*/
class ExternalTransport
{
public:

    virtual ~ExternalTransport () {} 

    //==============================================================================
    /**
        Try to make this client grab main external transport

        If you want to control transport from your filter, you should try to
        grab it by using this function.

        @param conditionalGrab      if true, you can grab the transport ONLY if
                                    no other applications have grabbed already.
                                    if false, you are forcing grab of the transport

        @return true if your client grabbed transport correctly
    */
    virtual bool grabTransport (const bool conditionalGrab = false) = 0;

    /**
        Release this client grabbing the main external transport

        This works only if you already grabbed the transport over all the other
        clients.
    */
    virtual void releaseTransport () = 0;

    /**
        Play the transport

        This will call the server to notify all the other attached clients that
        are listening for transport states. This only works if you already taken
        the transport.
    */
    virtual void playTransport () = 0;

    /**
        Stop main transport

        This will call the server to notify all the other attached clients that
        are listening for transport states. This only works if you already taken
        the transport.
    */
    virtual void stopTransport () = 0;

    /**
        Seek the transport to a specified frame number

        This will call the server to notify all the other attached clients that
        are listening for transport states. This only works if you already taken
        the transport.
    */
    virtual void seekTransportToFrame (const int frameToSeekAt) = 0;

    /**
        Set main tempo in beats per seconds
    */
    virtual void setTempo (const double bpmToSet) = 0;

    /**
        Set sync timeout in seconds
    */
    virtual void setSyncTimeout (const double secondsTimeout) = 0;
};

#endif
