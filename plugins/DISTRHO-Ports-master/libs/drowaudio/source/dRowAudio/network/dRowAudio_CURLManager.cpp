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
juce_ImplementSingleton (CURLManager);

CURLManager::CURLManager()
    : TimeSliceThread ("cURL Thread")
{
	CURLcode result = curl_global_init (CURL_GLOBAL_ALL);
    
    (void) result;
	jassert (result == CURLE_OK);
}

CURLManager::~CURLManager()
{
	curl_global_cleanup();
}

CURL* CURLManager::createEasyCurlHandle()
{
    return curl_easy_init();
}

void CURLManager::cleanUpEasyCurlHandle (CURL* handle)
{
	curl_easy_cleanup (handle);
	handle = nullptr;
}

StringArray CURLManager::getSupportedProtocols()
{
    if (curl_version_info_data* info = curl_version_info (CURLVERSION_NOW))
        return StringArray (info->protocols);
	
	return StringArray();
}

#endif
