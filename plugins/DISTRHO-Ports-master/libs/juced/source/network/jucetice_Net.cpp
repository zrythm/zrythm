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

#if 0

#if JUCE_MSVC
  #pragma warning (push)
  #pragma warning (disable: 4273)
#endif

namespace curlNamespace
{
#undef T
#if JUCETICE_INCLUDE_CURL_CODE
  #include "../dependancies/curl/include/curl/curl.h"
  #include "../dependancies/curl/lib/amigaos.c"
  #include "../dependancies/curl/lib/base64.c"
  #include "../dependancies/curl/lib/connect.c"
  #include "../dependancies/curl/lib/content_encoding.c"
  #include "../dependancies/curl/lib/cookie.c"
  #include "../dependancies/curl/lib/curl_addrinfo.c"
  #include "../dependancies/curl/lib/curl_sspi.c"
  #include "../dependancies/curl/lib/dict.c"
  #include "../dependancies/curl/lib/easy.c"
  #include "../dependancies/curl/lib/escape.c"
  #include "../dependancies/curl/lib/file.c"
  #include "../dependancies/curl/lib/formdata.c"
  #include "../dependancies/curl/lib/ftp.c"
  #include "../dependancies/curl/lib/getenv.c"
  #include "../dependancies/curl/lib/getinfo.c"
  #include "../dependancies/curl/lib/gtls.c"
  #include "../dependancies/curl/lib/hash.c"
  #include "../dependancies/curl/lib/hostares.c"
  #include "../dependancies/curl/lib/hostasyn.c"
  #include "../dependancies/curl/lib/hostip.c"
  #include "../dependancies/curl/lib/hostip4.c"
  #include "../dependancies/curl/lib/hostip6.c"
  #include "../dependancies/curl/lib/hostsyn.c"
  #include "../dependancies/curl/lib/hostthre.c"
  #include "../dependancies/curl/lib/http.c"
  #include "../dependancies/curl/lib/http_chunks.c"
  #include "../dependancies/curl/lib/http_digest.c"
  #include "../dependancies/curl/lib/http_negotiate.c"
  #include "../dependancies/curl/lib/http_ntlm.c"
  #include "../dependancies/curl/lib/if2ip.c"
  #include "../dependancies/curl/lib/inet_ntop.c"
  #include "../dependancies/curl/lib/inet_pton.c"
  #include "../dependancies/curl/lib/krb4.c"
  #include "../dependancies/curl/lib/krb5.c"
  #include "../dependancies/curl/lib/ldap.c"
  #include "../dependancies/curl/lib/llist.c"
  #include "../dependancies/curl/lib/md5.c"
  #include "../dependancies/curl/lib/memdebug.c"
  #include "../dependancies/curl/lib/mprintf.c"
  #include "../dependancies/curl/lib/multi.c"
  #include "../dependancies/curl/lib/netrc.c"
  #include "../dependancies/curl/lib/nss.c"
  #include "../dependancies/curl/lib/nwlib.c"
  #include "../dependancies/curl/lib/nwos.c"
  #include "../dependancies/curl/lib/parsedate.c"
  #include "../dependancies/curl/lib/progress.c"
  #include "../dependancies/curl/lib/qssl.c"
  #include "../dependancies/curl/lib/rawstr.c"
  #include "../dependancies/curl/lib/security.c"
  #include "../dependancies/curl/lib/select.c"
  #include "../dependancies/curl/lib/sendf.c"
  #include "../dependancies/curl/lib/share.c"
  #include "../dependancies/curl/lib/socks.c"
  #include "../dependancies/curl/lib/socks_gssapi.c"
  #include "../dependancies/curl/lib/socks_sspi.c"
  #include "../dependancies/curl/lib/speedcheck.c"
  #include "../dependancies/curl/lib/splay.c"
  #include "../dependancies/curl/lib/ssh.c"
  #include "../dependancies/curl/lib/sslgen.c"
  #include "../dependancies/curl/lib/ssluse.c"
  #include "../dependancies/curl/lib/strdup.c"
  #include "../dependancies/curl/lib/strequal.c"
  #include "../dependancies/curl/lib/strerror.c"
  #include "../dependancies/curl/lib/strtok.c"
  #include "../dependancies/curl/lib/strtoofft.c"
  #include "../dependancies/curl/lib/telnet.c"
  #include "../dependancies/curl/lib/tftp.c"
  #include "../dependancies/curl/lib/timeval.c"
  #include "../dependancies/curl/lib/transfer.c"
  #include "../dependancies/curl/lib/url.c"
  #include "../dependancies/curl/lib/version.c"
#else
  #include <curl.h>
#endif
#define T(stringLiteral)            JUCE_T(stringLiteral)
}

#if JUCE_MSVC
  #pragma warning (pop)
#endif

BEGIN_JUCE_NAMESPACE

using namespace curlNamespace;

//==============================================================================
namespace NetWrapperFunctions
{
    static int progressCallback (void* data, double dltotal, double dlnow, double ultotal, double ulnow)
    {
	    cURL* url = (cURL*) data;

	    if (url->threadShouldExit())
	    {
		    return (-1);
	    }

        // ScopedLock sl (cs);

	    for (int i = 0; i < url->getListenersCount(); i++)
	    {
		    cURLListener* listener = url->getListener (i);
		    if (listener)
		    {
			    if (listener->progress (dltotal, dlnow, ultotal, ulnow))
			    {
				    return (-1);
			    }
		    }
	    }

	    return (0);
    }

    static int writeFunction (void *ptr, size_t size, size_t nmemb, void *data)
    {
	    cURL* url = (cURL*) data;
	    if (url)
	    {
            // ScopedLock sl (cs);

		    for (int i = 0; i < url->getListenersCount (); i++)
		    {
			    cURLListener* listener = url->getListener (i);
			    if (listener)
			    {
			        if (listener->write (MemoryBlock (ptr, (size * nmemb))))
				    {
					    MemoryBlock *bl = url->getMemoryBlock();
					    const int currentPosition = url->getCurrentPosition();

					    bl->ensureSize ((size*nmemb) + currentPosition);
					    bl->copyFrom (ptr, currentPosition, (nmemb*size));
					    url->setCurrentPosition (bl->getSize());
				    }
			    }
		    }
	    }

	    return (nmemb * size);
    }

    CURLoption optionTranscode (const int option)
    {
#define WRAP_RETURN_OPTION(a) case a: { return a; }
        switch (option) {
            WRAP_RETURN_OPTION(CURLOPT_FILE)
            WRAP_RETURN_OPTION(CURLOPT_URL)
            WRAP_RETURN_OPTION(CURLOPT_PORT)
            WRAP_RETURN_OPTION(CURLOPT_PROXY)
            WRAP_RETURN_OPTION(CURLOPT_TIMEOUT)
            WRAP_RETURN_OPTION(CURLOPT_REFERER)
            WRAP_RETURN_OPTION(CURLOPT_FTPPORT)
            WRAP_RETURN_OPTION(CURLOPT_USERAGENT)

            WRAP_RETURN_OPTION(CURLOPT_USERPWD)
            WRAP_RETURN_OPTION(CURLOPT_PROXYUSERPWD)
            WRAP_RETURN_OPTION(CURLOPT_RANGE)
            WRAP_RETURN_OPTION(CURLOPT_ERRORBUFFER)
            WRAP_RETURN_OPTION(CURLOPT_WRITEFUNCTION)
            WRAP_RETURN_OPTION(CURLOPT_READFUNCTION)
            WRAP_RETURN_OPTION(CURLOPT_INFILE)
            WRAP_RETURN_OPTION(CURLOPT_INFILESIZE)
            WRAP_RETURN_OPTION(CURLOPT_POSTFIELDS)
            WRAP_RETURN_OPTION(CURLOPT_LOW_SPEED_LIMIT)
            WRAP_RETURN_OPTION(CURLOPT_LOW_SPEED_TIME)
            WRAP_RETURN_OPTION(CURLOPT_RESUME_FROM)
            WRAP_RETURN_OPTION(CURLOPT_COOKIE)
            WRAP_RETURN_OPTION(CURLOPT_HTTPHEADER)
            WRAP_RETURN_OPTION(CURLOPT_HTTPPOST)
            WRAP_RETURN_OPTION(CURLOPT_SSLCERT)
            WRAP_RETURN_OPTION(CURLOPT_KEYPASSWD)
            WRAP_RETURN_OPTION(CURLOPT_CRLF)
            WRAP_RETURN_OPTION(CURLOPT_QUOTE)
            WRAP_RETURN_OPTION(CURLOPT_WRITEHEADER)
            WRAP_RETURN_OPTION(CURLOPT_COOKIEFILE)
            WRAP_RETURN_OPTION(CURLOPT_SSLVERSION)
            WRAP_RETURN_OPTION(CURLOPT_TIMECONDITION)
            WRAP_RETURN_OPTION(CURLOPT_TIMEVALUE)
            WRAP_RETURN_OPTION(CURLOPT_CUSTOMREQUEST)
            WRAP_RETURN_OPTION(CURLOPT_STDERR)
            WRAP_RETURN_OPTION(CURLOPT_POSTQUOTE)
            WRAP_RETURN_OPTION(CURLOPT_WRITEINFO)
            WRAP_RETURN_OPTION(CURLOPT_VERBOSE)
            WRAP_RETURN_OPTION(CURLOPT_HEADER)
            WRAP_RETURN_OPTION(CURLOPT_NOPROGRESS)
            WRAP_RETURN_OPTION(CURLOPT_NOBODY)
            WRAP_RETURN_OPTION(CURLOPT_FAILONERROR)
            WRAP_RETURN_OPTION(CURLOPT_UPLOAD)
            WRAP_RETURN_OPTION(CURLOPT_POST)
            WRAP_RETURN_OPTION(CURLOPT_DIRLISTONLY)
            WRAP_RETURN_OPTION(CURLOPT_APPEND)
#if 0

  /* Specify whether to read the user+password from the .netrc or the URL.
   * This must be one of the CURL_NETRC_* enums below. */
  CINIT(NETRC, LONG, 51),

  CINIT(FOLLOWLOCATION, LONG, 52),  /* use Location: Luke! */

  CINIT(TRANSFERTEXT, LONG, 53), /* transfer data in text/ASCII format */
  CINIT(PUT, LONG, 54),          /* HTTP PUT */

  /* 55 = OBSOLETE */

  /* Function that will be called instead of the internal progress display
   * function. This function should be defined as the curl_progress_callback
   * prototype defines. */
  CINIT(PROGRESSFUNCTION, FUNCTIONPOINT, 56),

  /* Data passed to the progress callback */
  CINIT(PROGRESSDATA, OBJECTPOINT, 57),

  /* We want the referrer field set automatically when following locations */
  CINIT(AUTOREFERER, LONG, 58),

  /* Port of the proxy, can be set in the proxy string as well with:
     "[host]:[port]" */
  CINIT(PROXYPORT, LONG, 59),

  /* size of the POST input data, if strlen() is not good to use */
  CINIT(POSTFIELDSIZE, LONG, 60),

  /* tunnel non-http operations through a HTTP proxy */
  CINIT(HTTPPROXYTUNNEL, LONG, 61),

  /* Set the interface string to use as outgoing network interface */
  CINIT(INTERFACE, OBJECTPOINT, 62),

  /* Set the krb4/5 security level, this also enables krb4/5 awareness.  This
   * is a string, 'clear', 'safe', 'confidential' or 'private'.  If the string
   * is set but doesn't match one of these, 'private' will be used.  */
  CINIT(KRBLEVEL, OBJECTPOINT, 63),

  /* Set if we should verify the peer in ssl handshake, set 1 to verify. */
  CINIT(SSL_VERIFYPEER, LONG, 64),

  /* The CApath or CAfile used to validate the peer certificate
     this option is used only if SSL_VERIFYPEER is true */
  CINIT(CAINFO, OBJECTPOINT, 65),

  /* 66 = OBSOLETE */
  /* 67 = OBSOLETE */

  /* Maximum number of http redirects to follow */
  CINIT(MAXREDIRS, LONG, 68),

  /* Pass a long set to 1 to get the date of the requested document (if
     possible)! Pass a zero to shut it off. */
  CINIT(FILETIME, LONG, 69),

  /* This points to a linked list of telnet options */
  CINIT(TELNETOPTIONS, OBJECTPOINT, 70),

  /* Max amount of cached alive connections */
  CINIT(MAXCONNECTS, LONG, 71),

  /* What policy to use when closing connections when the cache is filled
     up */
  CINIT(CLOSEPOLICY, LONG, 72),

  /* 73 = OBSOLETE */

  /* Set to explicitly use a new connection for the upcoming transfer.
     Do not use this unless you're absolutely sure of this, as it makes the
     operation slower and is less friendly for the network. */
  CINIT(FRESH_CONNECT, LONG, 74),

  /* Set to explicitly forbid the upcoming transfer's connection to be re-used
     when done. Do not use this unless you're absolutely sure of this, as it
     makes the operation slower and is less friendly for the network. */
  CINIT(FORBID_REUSE, LONG, 75),

  /* Set to a file name that contains random data for libcurl to use to
     seed the random engine when doing SSL connects. */
  CINIT(RANDOM_FILE, OBJECTPOINT, 76),

  /* Set to the Entropy Gathering Daemon socket pathname */
  CINIT(EGDSOCKET, OBJECTPOINT, 77),

  /* Time-out connect operations after this amount of seconds, if connects
     are OK within this time, then fine... This only aborts the connect
     phase. [Only works on unix-style/SIGALRM operating systems] */
  CINIT(CONNECTTIMEOUT, LONG, 78),

  /* Function that will be called to store headers (instead of fwrite). The
   * parameters will use fwrite() syntax, make sure to follow them. */
  CINIT(HEADERFUNCTION, FUNCTIONPOINT, 79),

  /* Set this to force the HTTP request to get back to GET. Only really usable
     if POST, PUT or a custom request have been used first.
   */
  CINIT(HTTPGET, LONG, 80),

  /* Set if we should verify the Common name from the peer certificate in ssl
   * handshake, set 1 to check existence, 2 to ensure that it matches the
   * provided hostname. */
  CINIT(SSL_VERIFYHOST, LONG, 81),

  /* Specify which file name to write all known cookies in after completed
     operation. Set file name to "-" (dash) to make it go to stdout. */
  CINIT(COOKIEJAR, OBJECTPOINT, 82),

  /* Specify which SSL ciphers to use */
  CINIT(SSL_CIPHER_LIST, OBJECTPOINT, 83),

  /* Specify which HTTP version to use! This must be set to one of the
     CURL_HTTP_VERSION* enums set below. */
  CINIT(HTTP_VERSION, LONG, 84),

  /* Specifically switch on or off the FTP engine's use of the EPSV command. By
     default, that one will always be attempted before the more traditional
     PASV command. */
  CINIT(FTP_USE_EPSV, LONG, 85),

  /* type of the file keeping your SSL-certificate ("DER", "PEM", "ENG") */
  CINIT(SSLCERTTYPE, OBJECTPOINT, 86),

  /* name of the file keeping your private SSL-key */
  CINIT(SSLKEY, OBJECTPOINT, 87),

  /* type of the file keeping your private SSL-key ("DER", "PEM", "ENG") */
  CINIT(SSLKEYTYPE, OBJECTPOINT, 88),

  /* crypto engine for the SSL-sub system */
  CINIT(SSLENGINE, OBJECTPOINT, 89),

  /* set the crypto engine for the SSL-sub system as default
     the param has no meaning...
   */
  CINIT(SSLENGINE_DEFAULT, LONG, 90),

  /* Non-zero value means to use the global dns cache */
  CINIT(DNS_USE_GLOBAL_CACHE, LONG, 91), /* To become OBSOLETE soon */

  /* DNS cache timeout */
  CINIT(DNS_CACHE_TIMEOUT, LONG, 92),

  /* send linked-list of pre-transfer QUOTE commands */
  CINIT(PREQUOTE, OBJECTPOINT, 93),

  /* set the debug function */
  CINIT(DEBUGFUNCTION, FUNCTIONPOINT, 94),

  /* set the data for the debug function */
  CINIT(DEBUGDATA, OBJECTPOINT, 95),

  /* mark this as start of a cookie session */
  CINIT(COOKIESESSION, LONG, 96),

  /* The CApath directory used to validate the peer certificate
     this option is used only if SSL_VERIFYPEER is true */
  CINIT(CAPATH, OBJECTPOINT, 97),

  /* Instruct libcurl to use a smaller receive buffer */
  CINIT(BUFFERSIZE, LONG, 98),

  /* Instruct libcurl to not use any signal/alarm handlers, even when using
     timeouts. This option is useful for multi-threaded applications.
     See libcurl-the-guide for more background information. */
  CINIT(NOSIGNAL, LONG, 99),

  /* Provide a CURLShare for mutexing non-ts data */
  CINIT(SHARE, OBJECTPOINT, 100),

  /* indicates type of proxy. accepted values are CURLPROXY_HTTP (default),
     CURLPROXY_SOCKS4, CURLPROXY_SOCKS4A and CURLPROXY_SOCKS5. */
  CINIT(PROXYTYPE, LONG, 101),

  /* Set the Accept-Encoding string. Use this to tell a server you would like
     the response to be compressed. */
  CINIT(ENCODING, OBJECTPOINT, 102),

  /* Set pointer to private data */
  CINIT(PRIVATE, OBJECTPOINT, 103),

  /* Set aliases for HTTP 200 in the HTTP Response header */
  CINIT(HTTP200ALIASES, OBJECTPOINT, 104),

  /* Continue to send authentication (user+password) when following locations,
     even when hostname changed. This can potentially send off the name
     and password to whatever host the server decides. */
  CINIT(UNRESTRICTED_AUTH, LONG, 105),

  /* Specifically switch on or off the FTP engine's use of the EPRT command ( it
     also disables the LPRT attempt). By default, those ones will always be
     attempted before the good old traditional PORT command. */
  CINIT(FTP_USE_EPRT, LONG, 106),

  /* Set this to a bitmask value to enable the particular authentications
     methods you like. Use this in combination with CURLOPT_USERPWD.
     Note that setting multiple bits may cause extra network round-trips. */
  CINIT(HTTPAUTH, LONG, 107),

  /* Set the ssl context callback function, currently only for OpenSSL ssl_ctx
     in second argument. The function must be matching the
     curl_ssl_ctx_callback proto. */
  CINIT(SSL_CTX_FUNCTION, FUNCTIONPOINT, 108),

  /* Set the userdata for the ssl context callback function's third
     argument */
  CINIT(SSL_CTX_DATA, OBJECTPOINT, 109),

  /* FTP Option that causes missing dirs to be created on the remote server.
     In 7.19.4 we introduced the convenience enums for this option using the
     CURLFTP_CREATE_DIR prefix.
  */
  CINIT(FTP_CREATE_MISSING_DIRS, LONG, 110),

  /* Set this to a bitmask value to enable the particular authentications
     methods you like. Use this in combination with CURLOPT_PROXYUSERPWD.
     Note that setting multiple bits may cause extra network round-trips. */
  CINIT(PROXYAUTH, LONG, 111),

  /* FTP option that changes the timeout, in seconds, associated with
     getting a response.  This is different from transfer timeout time and
     essentially places a demand on the FTP server to acknowledge commands
     in a timely manner. */
  CINIT(FTP_RESPONSE_TIMEOUT, LONG, 112),

  /* Set this option to one of the CURL_IPRESOLVE_* defines (see below) to
     tell libcurl to resolve names to those IP versions only. This only has
     affect on systems with support for more than one, i.e IPv4 _and_ IPv6. */
  CINIT(IPRESOLVE, LONG, 113),

  /* Set this option to limit the size of a file that will be downloaded from
     an HTTP or FTP server.

     Note there is also _LARGE version which adds large file support for
     platforms which have larger off_t sizes.  See MAXFILESIZE_LARGE below. */
  CINIT(MAXFILESIZE, LONG, 114),

  /* See the comment for INFILESIZE above, but in short, specifies
   * the size of the file being uploaded.  -1 means unknown.
   */
  CINIT(INFILESIZE_LARGE, OFF_T, 115),

  /* Sets the continuation offset.  There is also a LONG version of this;
   * look above for RESUME_FROM.
   */
  CINIT(RESUME_FROM_LARGE, OFF_T, 116),

  /* Sets the maximum size of data that will be downloaded from
   * an HTTP or FTP server.  See MAXFILESIZE above for the LONG version.
   */
  CINIT(MAXFILESIZE_LARGE, OFF_T, 117),

  /* Set this option to the file name of your .netrc file you want libcurl
     to parse (using the CURLOPT_NETRC option). If not set, libcurl will do
     a poor attempt to find the user's home directory and check for a .netrc
     file in there. */
  CINIT(NETRC_FILE, OBJECTPOINT, 118),

  /* Enable SSL/TLS for FTP, pick one of:
     CURLFTPSSL_TRY     - try using SSL, proceed anyway otherwise
     CURLFTPSSL_CONTROL - SSL for the control connection or fail
     CURLFTPSSL_ALL     - SSL for all communication or fail
  */
  CINIT(USE_SSL, LONG, 119),

  /* The _LARGE version of the standard POSTFIELDSIZE option */
  CINIT(POSTFIELDSIZE_LARGE, OFF_T, 120),

  /* Enable/disable the TCP Nagle algorithm */
  CINIT(TCP_NODELAY, LONG, 121),

  /* 122 OBSOLETE, used in 7.12.3. Gone in 7.13.0 */
  /* 123 OBSOLETE. Gone in 7.16.0 */
  /* 124 OBSOLETE, used in 7.12.3. Gone in 7.13.0 */
  /* 125 OBSOLETE, used in 7.12.3. Gone in 7.13.0 */
  /* 126 OBSOLETE, used in 7.12.3. Gone in 7.13.0 */
  /* 127 OBSOLETE. Gone in 7.16.0 */
  /* 128 OBSOLETE. Gone in 7.16.0 */

  /* When FTP over SSL/TLS is selected (with CURLOPT_USE_SSL), this option
     can be used to change libcurl's default action which is to first try
     "AUTH SSL" and then "AUTH TLS" in this order, and proceed when a OK
     response has been received.

     Available parameters are:
     CURLFTPAUTH_DEFAULT - let libcurl decide
     CURLFTPAUTH_SSL     - try "AUTH SSL" first, then TLS
     CURLFTPAUTH_TLS     - try "AUTH TLS" first, then SSL
  */
  CINIT(FTPSSLAUTH, LONG, 129),

  CINIT(IOCTLFUNCTION, FUNCTIONPOINT, 130),
  CINIT(IOCTLDATA, OBJECTPOINT, 131),

  /* 132 OBSOLETE. Gone in 7.16.0 */
  /* 133 OBSOLETE. Gone in 7.16.0 */

  /* zero terminated string for pass on to the FTP server when asked for
     "account" info */
  CINIT(FTP_ACCOUNT, OBJECTPOINT, 134),

  /* feed cookies into cookie engine */
  CINIT(COOKIELIST, OBJECTPOINT, 135),

  /* ignore Content-Length */
  CINIT(IGNORE_CONTENT_LENGTH, LONG, 136),

  /* Set to non-zero to skip the IP address received in a 227 PASV FTP server
     response. Typically used for FTP-SSL purposes but is not restricted to
     that. libcurl will then instead use the same IP address it used for the
     control connection. */
  CINIT(FTP_SKIP_PASV_IP, LONG, 137),

  /* Select "file method" to use when doing FTP, see the curl_ftpmethod
     above. */
  CINIT(FTP_FILEMETHOD, LONG, 138),

  /* Local port number to bind the socket to */
  CINIT(LOCALPORT, LONG, 139),

  /* Number of ports to try, including the first one set with LOCALPORT.
     Thus, setting it to 1 will make no additional attempts but the first.
  */
  CINIT(LOCALPORTRANGE, LONG, 140),

  /* no transfer, set up connection and let application use the socket by
     extracting it with CURLINFO_LASTSOCKET */
  CINIT(CONNECT_ONLY, LONG, 141),

  /* Function that will be called to convert from the
     network encoding (instead of using the iconv calls in libcurl) */
  CINIT(CONV_FROM_NETWORK_FUNCTION, FUNCTIONPOINT, 142),

  /* Function that will be called to convert to the
     network encoding (instead of using the iconv calls in libcurl) */
  CINIT(CONV_TO_NETWORK_FUNCTION, FUNCTIONPOINT, 143),

  /* Function that will be called to convert from UTF8
     (instead of using the iconv calls in libcurl)
     Note that this is used only for SSL certificate processing */
  CINIT(CONV_FROM_UTF8_FUNCTION, FUNCTIONPOINT, 144),

  /* if the connection proceeds too quickly then need to slow it down */
  /* limit-rate: maximum number of bytes per second to send or receive */
  CINIT(MAX_SEND_SPEED_LARGE, OFF_T, 145),
  CINIT(MAX_RECV_SPEED_LARGE, OFF_T, 146),

  /* Pointer to command string to send if USER/PASS fails. */
  CINIT(FTP_ALTERNATIVE_TO_USER, OBJECTPOINT, 147),

  /* callback function for setting socket options */
  CINIT(SOCKOPTFUNCTION, FUNCTIONPOINT, 148),
  CINIT(SOCKOPTDATA, OBJECTPOINT, 149),

  /* set to 0 to disable session ID re-use for this transfer, default is
     enabled (== 1) */
  CINIT(SSL_SESSIONID_CACHE, LONG, 150),

  /* allowed SSH authentication methods */
  CINIT(SSH_AUTH_TYPES, LONG, 151),

  /* Used by scp/sftp to do public/private key authentication */
  CINIT(SSH_PUBLIC_KEYFILE, OBJECTPOINT, 152),
  CINIT(SSH_PRIVATE_KEYFILE, OBJECTPOINT, 153),

  /* Send CCC (Clear Command Channel) after authentication */
  CINIT(FTP_SSL_CCC, LONG, 154),

  /* Same as TIMEOUT and CONNECTTIMEOUT, but with ms resolution */
  CINIT(TIMEOUT_MS, LONG, 155),
  CINIT(CONNECTTIMEOUT_MS, LONG, 156),

  /* set to zero to disable the libcurl's decoding and thus pass the raw body
     data to the application even when it is encoded/compressed */
  CINIT(HTTP_TRANSFER_DECODING, LONG, 157),
  CINIT(HTTP_CONTENT_DECODING, LONG, 158),

  /* Permission used when creating new files and directories on the remote
     server for protocols that support it, SFTP/SCP/FILE */
  CINIT(NEW_FILE_PERMS, LONG, 159),
  CINIT(NEW_DIRECTORY_PERMS, LONG, 160),

  /* Set the behaviour of POST when redirecting. Values must be set to one
     of CURL_REDIR* defines below. This used to be called CURLOPT_POST301 */
  CINIT(POSTREDIR, LONG, 161),

  /* used by scp/sftp to verify the host's public key */
  CINIT(SSH_HOST_PUBLIC_KEY_MD5, OBJECTPOINT, 162),

  /* Callback function for opening socket (instead of socket(2)). Optionally,
     callback is able change the address or refuse to connect returning
     CURL_SOCKET_BAD.  The callback should have type
     curl_opensocket_callback */
  CINIT(OPENSOCKETFUNCTION, FUNCTIONPOINT, 163),
  CINIT(OPENSOCKETDATA, OBJECTPOINT, 164),

  /* POST volatile input fields. */
  CINIT(COPYPOSTFIELDS, OBJECTPOINT, 165),

  /* set transfer mode (;type=<a|i>) when doing FTP via an HTTP proxy */
  CINIT(PROXY_TRANSFER_MODE, LONG, 166),

  /* Callback function for seeking in the input stream */
  CINIT(SEEKFUNCTION, FUNCTIONPOINT, 167),
  CINIT(SEEKDATA, OBJECTPOINT, 168),

  /* CRL file */
  CINIT(CRLFILE, OBJECTPOINT, 169),

  /* Issuer certificate */
  CINIT(ISSUERCERT, OBJECTPOINT, 170),

  /* (IPv6) Address scope */
  CINIT(ADDRESS_SCOPE, LONG, 171),

  /* Collect certificate chain info and allow it to get retrievable with
     CURLINFO_CERTINFO after the transfer is complete. (Unfortunately) only
     working with OpenSSL-powered builds. */
  CINIT(CERTINFO, LONG, 172),

  /* "name" and "pwd" to use when fetching. */
  CINIT(USERNAME, OBJECTPOINT, 173),
  CINIT(PASSWORD, OBJECTPOINT, 174),

    /* "name" and "pwd" to use with Proxy when fetching. */
  CINIT(PROXYUSERNAME, OBJECTPOINT, 175),
  CINIT(PROXYPASSWORD, OBJECTPOINT, 176),

  /* Comma separated list of hostnames defining no-proxy zones. These should
     match both hostnames directly, and hostnames within a domain. For
     example, local.com will match local.com and www.local.com, but NOT
     notlocal.com or www.notlocal.com. For compatibility with other
     implementations of this, .local.com will be considered to be the same as
     local.com. A single * is the only valid wildcard, and effectively
     disables the use of proxy. */
  CINIT(NOPROXY, OBJECTPOINT, 177),

  /* block size for TFTP transfers */
  CINIT(TFTP_BLKSIZE, LONG, 178),

  /* Socks Service */
  CINIT(SOCKS5_GSSAPI_SERVICE, LONG, 179),

  /* Socks Service */
  CINIT(SOCKS5_GSSAPI_NEC, LONG, 180),

  /* set the bitmask for the protocols that are allowed to be used for the
     transfer, which thus helps the app which takes URLs from users or other
     external inputs and want to restrict what protocol(s) to deal
     with. Defaults to CURLPROTO_ALL. */
  CINIT(PROTOCOLS, LONG, 181),

  /* set the bitmask for the protocols that libcurl is allowed to follow to,
     as a subset of the CURLOPT_PROTOCOLS ones. That means the protocol needs
     to be set in both bitmasks to be allowed to get redirected to. Defaults
     to all protocols except FILE and SCP. */
  CINIT(REDIR_PROTOCOLS, LONG, 182),
#endif
        default:
            break;
        }

        jassertfalse; // should never reach here !
        
        return CURLOPT_FILE;
    }
  
}

//==============================================================================
cURL::cURL (const NetworkOperationMode& newMode)
    : Thread (T("CurlThread")),
      mode (newMode),
      currentError (String()),
      curl (0)
{
	bl = new MemoryBlock (0);
}

cURL::~cURL()
{
	curl_easy_cleanup (curl);
	deleteAndZero (bl);
}

//==============================================================================
bool cURL::initialize()
{
#if JUCE_WIN32
	curl_global_init (CURL_GLOBAL_WIN32);
#else
	curl_global_init (CURL_GLOBAL_ALL);
#endif
	curl	= curl_easy_init();
	CURLcode ret;
    
	if (! curl)
	{
		return (false);
	}

	if ((ret = curl_easy_setopt ((CURL*) curl, CURLOPT_NOPROGRESS, FALSE)))
	{
		currentError = curl_easy_strerror (ret);
		return (false);
	}

	if ((ret = curl_easy_setopt ((CURL*) curl, CURLOPT_PROGRESSFUNCTION, NetWrapperFunctions::progressCallback)))
    {
		currentError = curl_easy_strerror (ret);
		return (false);
	}
        
	if ((ret = curl_easy_setopt ((CURL*) curl, CURLOPT_PROGRESSDATA, this)))
	{
		currentError = curl_easy_strerror(ret);
		return (false);
	}

	if ((ret = curl_easy_setopt ((CURL*) curl, CURLOPT_WRITEFUNCTION, NetWrapperFunctions::writeFunction)))
	{
		currentError = curl_easy_strerror(ret);
		return (false);
	}

	if ((ret = curl_easy_setopt ((CURL*) curl, CURLOPT_WRITEDATA, this)))
	{
		currentError = curl_easy_strerror(ret);
		return (false);
	}
	
	return (true);
}

//==============================================================================
const String cURL::getErrorString()
{
	return (currentError);
}

//==============================================================================
bool cURL::setOption (const int curlOption, const int curlValue)
{
	CURLcode ret;

	if ((ret = curl_easy_setopt ((CURL*) curl, NetWrapperFunctions::optionTranscode (curlOption), curlValue)))
    {
		currentError = curl_easy_strerror(ret);
		return (false);
	}
	else
	{
		return (true);
	}
}

bool cURL::setOption (const int curlOption, const String curlValue)
{
	CURLcode ret;

	if ((ret = curl_easy_setopt ((CURL*) curl, NetWrapperFunctions::optionTranscode (curlOption), curlValue.toUTF8())))
    {
		currentError = curl_easy_strerror(ret);
		return (false);
	}
	else
	{
		return (true);
	}
}

//==============================================================================
void cURL::addListener (cURLListener *ptr)
{
	ScopedLock sl (cs);
	listeners.addIfNotAlreadyThere (ptr);
}

void cURL::removeListener (cURLListener *ptr)
{
	ScopedLock sl (cs);
	listeners.removeValue (ptr);
}

cURLListener* cURL::getListener (const int index)
{
    ScopedLock sl (cs);
    return (cURLListener*) listeners.getUnchecked (index);
}

int  cURL::getListenersCount () const
{
    ScopedLock sl (cs);
    return listeners.size ();
}

//==============================================================================
bool cURL::startTransfer()
{
	currentPosition = 0;
	bl->setSize (0, false);

	notifyTransferStart();

	if (mode == AsyncMode && ! isThreadRunning())
	{
		startThread();
		return (true);
	}
	
	if (curl)
	{
		Logger::writeToLog (T("cURL::startTransfer()"));

		CURLcode ret;
		if ((ret = curl_easy_perform ((CURL*) curl)))
		{
			currentError = curl_easy_strerror (ret);
			
            notifyTransferEnd (false);
			return (false);
		}
		else
		{
		    notifyTransferEnd (false);
			return (true);
		}
	}
	else
	{
		currentError = T("CURL not initiazlied");
		return (false);
	}
}

//==============================================================================
void cURL::run()
{
	if (mode == AsyncMode)
	{
		if (curl)
		{
			CURLcode ret;
			if ((ret = curl_easy_perform ((CURL*) curl)))
			{
				currentError = curl_easy_strerror (ret);
                notifyTransferEnd (true);
				return;
			}
			else
			{
                notifyTransferEnd (false);
				return;
			}
		}
		else
		{
			currentError = T("CURL not initiazlied");
		    notifyTransferEnd (true);
			return;
		}
	}
	else
	{
		currentError = T("CURL is not in ASYNC mode");
		return;
	}
}

//==============================================================================
MemoryBlock cURL::getRawData()
{
	return (*bl);
}

String cURL::getStringData()
{
	return String::fromUTF8 ((uint8*) bl->getData(), bl->getSize());
}

const uint32 cURL::getDataSize()
{
	return bl->getSize();
}

//==============================================================================
void cURL::setMode (const cURL::NetworkOperationMode& shouldBeAsync)
{
	mode = shouldBeAsync;
}

const cURL::NetworkOperationMode cURL::getMode()
{
	return mode;
}

//==============================================================================
/*
bool cURL::getInfo (const int curlOption, String &stringValue)
{
	CURLcode ret;
	if (ret = curl_easy_getinfo ((CURL*) curl, NetWrapperFunctions::optionTranscode (curlOption), stringValue))
	{
		currentError = curl_easy_strerror(ret);
		return (false);
	}

	return (true);
}

bool cURL::getInfo (const int curlOption, double &doubleValue)
{
	CURLcode ret;
	if (ret = curl_easy_getinfo ((CURL*) curl, NetWrapperFunctions::optionTranscode (curlOption), doubleValue))
	{
		currentError = curl_easy_strerror(ret);
		return (false);
	}

	return (true);
}

bool cURL::getInfo (const int curlOption, long &longValue)
{
	CURLcode ret;
	if (ret = curl_easy_getinfo ((CURL*) curl, NetWrapperFunctions::optionTranscode (curlOption), longValue))
	{
		currentError = curl_easy_strerror(ret);
		return (false);
	}

	return (true);
}
*/

//==============================================================================
MemoryBlock* cURL::getMemoryBlock()
{
	return bl;
}

const uint32 cURL::getCurrentPosition()
{
	return currentPosition;
}

void cURL::setCurrentPosition (const uint32 newPosition)
{
	currentPosition = newPosition;
}

void cURL::notifyTransferStart ()
{
    ScopedLock sl (cs);

    for (int i = 0; i < getListenersCount (); i++)
	{
		cURLListener* listener = getListener (i);
		if (listener)
			listener->transferStart ();
	}
}

void cURL::notifyTransferEnd (const bool errorIndicator)
{
    ScopedLock sl (cs);

    for (int i = 0; i < getListenersCount (); i++)
	{
		cURLListener* listener = getListener (i);
		if (listener)
			listener->transferEnd (errorIndicator);
	}
}

END_JUCE_NAMESPACE

#endif
