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

#ifdef WIN32
  #include <winsock.h>
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <sys/select.h>
  #include <netinet/in.h>
  #include <netdb.h>
#endif

BEGIN_JUCE_NAMESPACE

//==============================================================================
UDPSocket::UDPSocket ()
  : port (0)
{
    SocketSetup::getInstance();

    sock = socket (AF_INET, SOCK_DGRAM, 0);
}

UDPSocket::UDPSocket (const String& address, const short port)
{
    this->address = address;
    this->port = port;

    SocketSetup::getInstance();

    sock = socket (AF_INET, SOCK_DGRAM, 0);
}

UDPSocket::~UDPSocket ()
{
#ifdef WIN32
    closesocket (sock);
#else
    close (sock);
#endif
}

//==============================================================================
void UDPSocket::setAddress (const String& address)
{
    this->address = address;
}

void UDPSocket::setPort (const short port)
{
    this->port = port;
}

//==============================================================================
void UDPSocket::bindSocket ()
{
    sockaddr_in localAddress;

    localAddress.sin_family = AF_INET;
    localAddress.sin_port = htons(port);
    localAddress.sin_addr.s_addr = INADDR_ANY;
    memset (&(localAddress.sin_zero), 0, 8);

    bind (sock, (sockaddr *)(&localAddress), sizeof(sockaddr_in));
}

//==============================================================================
void UDPSocket::sendData (char *data, const long size)
{
    sockaddr_in destAddress;
    hostent *he = gethostbyname ((const char*) address.toUTF8());

    destAddress.sin_family = AF_INET;
    destAddress.sin_port = htons (port);
    /*destAddress.sin_addr = *((struct in_addr *)gethostbyaddr(inet_addr(address.c_str()),
                                                             (address.length()+1),
                                                             0)->h_addr_list);*/
    /*tempint = inet_addr(address.c_str());
    destAddress.sin_addr = *(in_addr *)(&tempint);*/
    destAddress.sin_addr = *(in_addr *)he->h_addr;
    memset (&(destAddress.sin_zero), '\0', 8);

    sendto(sock,
           data,
           size,
           0,
           reinterpret_cast<sockaddr *>(&destAddress),
           sizeof(sockaddr));
}

//==============================================================================
char *UDPSocket::getData (long& size)
{
    int bytesReceived = 0;
    timeval timeToWait;
    fd_set setToCheck;
    sockaddr_in receivedAddress;

    timeToWait.tv_sec = 0;
    timeToWait.tv_usec = 2500; // 1/4 of a second.

    FD_ZERO(&setToCheck);
    FD_SET(sock, &setToCheck);

    //Use select so that we can timeout to check if we're supposed to stop the
    //thread yet.
#ifdef WIN32
    if(select(1, &setToCheck, NULL, NULL, &timeToWait) == SOCKET_ERROR)
    {
        switch(WSAGetLastError())
        {
            case WSANOTINITIALISED:
                strcpy(receiveBuffer, "Not Initialised");
                break;
            case WSAEFAULT:
                strcpy(receiveBuffer, "Fault");
                break;
            case WSAENETDOWN:
                strcpy(receiveBuffer, "Network subsystem has failed.");
                break;
            case WSAEINVAL:
                strcpy(receiveBuffer, "Invalid parameter.");
                break;
            case WSAEINTR:
                strcpy(receiveBuffer, "Blocking socket was cancelled.");
                break;
            case WSAEINPROGRESS:
                strcpy(receiveBuffer, "Blocking socket in progress.");
                break;
            case WSAENOTSOCK:
                strcpy(receiveBuffer, "Not a socket.");
                break;
        }
        size = -1;
        return receiveBuffer;
    }
#else
    if (select((sock+1), &setToCheck, NULL, NULL, &timeToWait) == -1)
    {
        /*switch(errno)
        {
            case EBADF:
                strcpy(receiveBuffer, "Invalid file descriptor.");
                break;
            case EINTR:
                strcpy(receiveBuffer, "Non blocked signal was caught.");
                break;
            case EINVAL:
                strcpy(receiveBuffer, "Invalid time value.");
                break;
            case ENOMEM:
                strcpy(receiveBuffer, "Not enough memory.");
                break;
        }*/
        size = -1;
        return receiveBuffer;
    }
#endif

    if(FD_ISSET(sock, &setToCheck))
    {
        size = sizeof(sockaddr_in);
#ifndef WIN32
        bytesReceived = recvfrom(sock,
                                 receiveBuffer,
                                 (MaxBufferSize-1),
                                 0,
                                 (sockaddr *)(&receivedAddress),
                                 (unsigned int *)(&size));
#else
        bytesReceived = recvfrom(sock,
                                 receiveBuffer,
                                 (MaxBufferSize-1),
                                 0,
                                 (sockaddr *)(&receivedAddress),
                                 (int *)(&size));
#endif

        if(bytesReceived > 0)
        {
            size = bytesReceived;
            return receiveBuffer;
        }
        else
        {
            size = -1;
            strcpy(receiveBuffer, "Received an empty packet?");
            return receiveBuffer;
        }
    }
    else
    {
        size = -1;
        return 0;
    }
}

//==============================================================================
SocketSetup *SocketSetup::getInstance()
{
    static SocketSetup instance;

    return &instance;
}

SocketSetup::SocketSetup()
{
#ifdef WIN32
    WSADATA wsaData;   // if this doesn't work

    WSAStartup(MAKEWORD(1, 1), &wsaData);
#endif
}

SocketSetup::~SocketSetup()
{
#ifdef WIN32
    WSACleanup();
#endif
}

END_JUCE_NAMESPACE
