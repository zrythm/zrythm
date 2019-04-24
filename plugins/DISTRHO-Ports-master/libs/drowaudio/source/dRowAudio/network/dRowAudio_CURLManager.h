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

#ifndef __DROWAUDIO_CURLMANAGER_H__
#define __DROWAUDIO_CURLMANAGER_H__

#if DROWAUDIO_USE_CURL || DOXYGEN

}
typedef void CURL;
namespace drow {

//==============================================================================
class CURLManager : public TimeSliceThread,
					public DeletedAtShutdown
{
public:
	//==============================================================================
	juce_DeclareSingleton (CURLManager, true);
	
	CURLManager();
	
	~CURLManager();
	
	//==============================================================================
	/**	Creates a new easy curl session handle.
		This simply creates the handle for you, it is the caller's responsibility
		to clean up when the handle is no longer needed. This can be done with
		cleanUpEasyCurlHandle().
	 */
	CURL* createEasyCurlHandle();
	
	/**	Cleans up an easy curl session for you.
		You can pass this a handle generated with createEasyCurlHandle() to clean
		up any resources associated with it. Be careful not to use the handle after
		calling this function as it will be a nullptr.
	 */
	void cleanUpEasyCurlHandle (CURL* handle);
	
	/**	Returns a list of the supported protocols.
	 */
	StringArray getSupportedProtocols();
    
private:
	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CURLManager);
};

#endif
#endif  // __DROWAUDIO_CURLMANAGER_H__