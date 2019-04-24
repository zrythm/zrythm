/*
  ==============================================================================

  This file is part of the dRowAudio JUCE module
  Copyright 2004-13 by dRowAudio.

  ------------------------------------------------------------------------------

  dRowAudio is provided under the terms of The MIT License (MIT):

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
  SOFTWARE.

  ==============================================================================
*/

#if DROWAUDIO_USE_CURL

} //namespace drow

#if JUCE_WINDOWS
 #include "curl/include/curl/curl.h"
#else
 #include <curl/curl.h>
#endif

namespace drow {

//==============================================================================
CURLEasySession::CURLEasySession()
    : handle      (CURLManager::getInstance()->createEasyCurlHandle()),
	  remotePath  (String()),
	  progress    (1.0f)
{
	enableFullDebugging (true);
	curl_easy_setopt (handle, CURLOPT_NOPROGRESS, false);
}

CURLEasySession::CURLEasySession (String localPath,
                                  String remotePath,
                                  bool upload,
                                  String username,
                                  String password)
    : handle      (CURLManager::getInstance()->createEasyCurlHandle())
{
	handle = CURLManager::getInstance()->createEasyCurlHandle();
	enableFullDebugging (true);
	curl_easy_setopt (handle, CURLOPT_NOPROGRESS, false);

	setLocalFile (localPath);
	setRemotePath (remotePath);
	setUserNameAndPassword (username, password);
	beginTransfer (upload);
}

CURLEasySession::~CURLEasySession()
{
	CURLManager::getInstance()->removeTimeSliceClient (this);
	if (CURLManager::getInstance()->getNumClients() == 0)
		CURLManager::getInstance()->stopThread (1000);
	
	CURLManager::getInstance()->cleanUpEasyCurlHandle (handle);
}

//==============================================================================
void CURLEasySession::setInputStream (InputStream* newInputStream)
{
    localFile = File();
    inputStream = newInputStream;
}

void CURLEasySession::setLocalFile (File newLocalFile)
{
	localFile = newLocalFile;
    inputStream = localFile.createInputStream();
}

void CURLEasySession::setRemotePath (String newRemotePath)
{
	remotePath = newRemotePath;

    if (remotePath.getLastCharacters (1) == "/")
        remotePath  = remotePath << localFile.getFileName();

	curl_easy_setopt (handle, CURLOPT_URL, remotePath.toUTF8().getAddress());
}

void CURLEasySession::setUserNameAndPassword (String username, String password)
{
	userNameAndPassword = username << ":" << password;
	curl_easy_setopt (handle, CURLOPT_USERPWD, userNameAndPassword.toUTF8().getAddress());
}

//==============================================================================
String CURLEasySession::getCurrentWorkingDirectory()
{
	char url[1000];
	
	CURLcode res = curl_easy_getinfo (handle, CURLINFO_EFFECTIVE_URL, url);
	
	if (res == CURLE_OK && CharPointer_ASCII::isValidString (url, 1000))
		return String (url);
	else
		return String();
}

StringArray CURLEasySession::getDirectoryListing()
{
    String remoteUrl (remotePath.upToLastOccurrenceOf ("/", true, false));
	curl_easy_setopt (handle, CURLOPT_URL, remoteUrl.toUTF8().getAddress());
    
	directoryContentsList.setSize (0);
	
	curl_easy_setopt (handle, CURLOPT_PROGRESSFUNCTION, 0L);
	curl_easy_setopt (handle, CURLOPT_UPLOAD, 0L);
	curl_easy_setopt (handle, CURLOPT_DIRLISTONLY, 1L);
	curl_easy_setopt (handle, CURLOPT_WRITEDATA, this);
	curl_easy_setopt (handle, CURLOPT_WRITEFUNCTION, directoryListingCallback);
    
	// perform  the tranfer
	progress = 0.0f;
	CURLcode result = curl_easy_perform (handle);
	reset();	
	
	if (result == CURLE_OK)
	{
		StringArray list;
		list.addLines (directoryContentsList.toString().trim());
        
		return list;
	}
	else 
    {
		return StringArray (curl_easy_strerror (result));
	}				
}

// not yet ready
//String CURLEasySession::getContentType()
//{
//    char *ct;
//
//    CURLcode result = curl_easy_getinfo (handle, CURLINFO_CONTENT_TYPE, &ct);
//    
//    if (CURLE_OK == result)// && ct != nullptr)
//    {
//        DBG("CURLE_OK: " + remotePath);
//        return ct;
//    }
//    else
//    {
//        DBG("CURLE_NOT_OK");
//        return String();
//    }
//}

//==============================================================================
void CURLEasySession::enableFullDebugging (bool shouldEnableFullDebugging)
{
	curl_easy_setopt (handle, CURLOPT_VERBOSE, shouldEnableFullDebugging ? 1L : 0L);
}

void CURLEasySession::beginTransfer (bool transferIsUpload, bool performOnBackgroundThread)
{
	isUpload = transferIsUpload;
    shouldStopTransfer = false;
    
    if (performOnBackgroundThread)
    {
        CURLManager::getInstance()->addTimeSliceClient (this);
        CURLManager::getInstance()->startThread();
    }
    else
    {
        performTransfer (isUpload);
    }
}

void CURLEasySession::stopTransfer()
{
    shouldStopTransfer = true;
}

void CURLEasySession::reset()
{
	curl_easy_reset (handle);
	curl_easy_setopt (handle, CURLOPT_URL, remotePath.toUTF8().getAddress());
	curl_easy_setopt (handle, CURLOPT_USERPWD, userNameAndPassword.toUTF8().getAddress());
	curl_easy_setopt (handle, CURLOPT_NOPROGRESS, false);
}

//==============================================================================
void CURLEasySession::addListener (CURLEasySession::Listener* const listener)
{
    listeners.add (listener);
}

void CURLEasySession::removeListener (CURLEasySession::Listener* const listener)
{
    listeners.remove (listener);
}

//==============================================================================
int CURLEasySession::useTimeSlice()
{
    performTransfer (isUpload);

	return -1;
}

//==============================================================================
size_t CURLEasySession::writeCallback (void* sourcePointer, size_t blockSize, size_t numBlocks, CURLEasySession* session)
{
	if (session != nullptr)
	{
		if (session->outputStream->failedToOpen())
		{
            /* failure, can't open file to write */ 
			return ! (blockSize * numBlocks); // return a value not equal to (blockSize * numBlocks)
		}
		
		session->outputStream->write (sourcePointer, blockSize * numBlocks);
		return blockSize * numBlocks;
	}
	
	return ! (blockSize * numBlocks); // return a value not equal to (blockSize * numBlocks)
}

size_t CURLEasySession::readCallback (void* destinationPointer, size_t blockSize, size_t numBlocks, CURLEasySession* session)
{
	if (session != nullptr)
	{
		if (session->inputStream.get() == nullptr)
		{
			return CURL_READFUNC_ABORT; /* failure, can't open file to read */ 
		}
		
		return session->inputStream->read (destinationPointer, blockSize * numBlocks);
	}
	
	return CURL_READFUNC_ABORT;
}

size_t CURLEasySession::directoryListingCallback (void* sourcePointer, size_t blockSize, size_t numBlocks, CURLEasySession* session)
{
	if (session != nullptr)
	{
		session->directoryContentsList.append (sourcePointer, (int) (blockSize * numBlocks));

		return blockSize * numBlocks;
	}
	
	return ! (blockSize * numBlocks); // return a positive value not equal to (blockSize * numBlocks)
}

int CURLEasySession::internalProgressCallback (CURLEasySession* session, double dltotal, double dlnow, double /*ultotal*/, double ulnow)
{
	session->progress = (float) (session->isUpload ? (ulnow / session->inputStream->getTotalLength()) : (dlnow / dltotal));
	
    session->listeners.call (&CURLEasySession::Listener::transferProgressUpdate, session);
    
	return (int) session->shouldStopTransfer;
}

//==============================================================================
int CURLEasySession::performTransfer (bool transferIsUpload)
{
	curl_easy_setopt (handle, CURLOPT_URL, remotePath.toUTF8().getAddress());
    curl_easy_setopt (handle, CURLOPT_UPLOAD, (long) transferIsUpload);
    curl_easy_setopt (handle, CURLOPT_PROGRESSDATA, this);
	curl_easy_setopt (handle, CURLOPT_PROGRESSFUNCTION, internalProgressCallback);
    
    if (transferIsUpload == true)
	{
		// sets the pointer to be passed to the read callback
		curl_easy_setopt (handle, CURLOPT_READDATA, this);
		curl_easy_setopt (handle, CURLOPT_READFUNCTION, readCallback);
        inputStream->setPosition (0);
	}
	else
	{
		// sets the pointer to be passed to the write callback
		curl_easy_setopt (handle, CURLOPT_WRITEDATA, this);
		curl_easy_setopt (handle, CURLOPT_WRITEFUNCTION, writeCallback);
        
		// create local file to recieve transfer
		if (localFile.existsAsFile())
			localFile = localFile.getNonexistentSibling();
        
        outputStream = localFile.createOutputStream();
	}
    
	//perform the transfer
	progress = 0.0f;
    listeners.call (&CURLEasySession::Listener::transferAboutToStart, this);
	CURLcode result = curl_easy_perform (handle);
	
	// delete the streams to flush the buffers
	outputStream = nullptr;
    listeners.call (&CURLEasySession::Listener::transferEnded, this);
    
    return result;
}



#endif