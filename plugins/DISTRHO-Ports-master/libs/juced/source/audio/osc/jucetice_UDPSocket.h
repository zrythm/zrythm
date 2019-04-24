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

#ifndef __JUCETICE_UDPSOCKET_HEADER__
#define __JUCETICE_UDPSOCKET_HEADER__

//==============================================================================
/**
        Class to handle UDP network communication.
*/
class UDPSocket
{
public:

    //==============================================================================
    /** Default constructor

        This version is intended for keeping a socket open for a fairly long
        time.
    */
    UDPSocket();

    /** Constructor.

        This version is intended for times when you just want to quickly open a
        socket and send something (i.e. in a method that doesn't get called
        often).

        @param address           The address to send data to.
        @param port              The port to send data from/to.
    */
    UDPSocket (const String& address, const short port);

    /** Destructor */
    ~UDPSocket();

    //==============================================================================
    /** Sets the address to send data to */
    void setAddress (const String& address);

    /** Sets the port to send data from/to */
    void setPort (const short port);

    //==============================================================================
    /** Binds sock to the currently-set port.

        You must call this before you call getData().
    */
    void bindSocket();

    //==============================================================================
    /** Sends data to previously-set address/port.

        @param data                 The data to send (a contiguous block of bytes).
        @param size                 The size of the data to send.
    */
    void sendData (char *data, const long size);

    /** Returns a data packet sent to us.

        On return, size will be filled with the size of the packet, or -1 if it
        failed.
    */
    char *getData (long& size);

private:

    /** The address to send data to. */
    String address;

    /** The port to send data from/to. */
    short port;

    /** Our socket. */
    int sock;

    enum
    {
        MaxBufferSize = 16384  // 16kB should be enough?
    };

    /** Buffer to receive any data sent to us. */
    char receiveBuffer[MaxBufferSize];
};


//==============================================================================
/**
    Singleton class used to set up socket stuff on Windows.

    We need this to make sure we're set up properly before we start sending
    data, and also so that we don't end up calling it more than once.

*/
class SocketSetup
{
public:

    /** Returns the single active instance of the class.

        If the class is not yet instantiated, constructs it.  Since the actual
        setup is done in the constructor, the returned pointer probably isn't
        going to be any use...
    */
    static SocketSetup *getInstance();

    /** Always remember to delete the singleton when you're done with it */
    ~SocketSetup();

private:

    /** Constructor */
    SocketSetup();

    /** Pointer to the single instance of this class */
    static SocketSetup *instance;
};

#endif
