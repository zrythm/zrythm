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

#ifndef __JUCETICE_NET_HEADER__
#define __JUCETICE_NET_HEADER__

#if 0

//==============================================================================
class cURLListener
{
public:
	virtual ~cURLListener() {}

	virtual int progress (double dltotal, double dlnow, double ultotal, double ulnow)=0;
	virtual void transferStart ()=0;
	virtual void transferEnd (const bool errorIndicator)=0;
	virtual const bool write (const MemoryBlock &dataToWrite)=0;

protected:
	cURLListener() {}
};


//==============================================================================
class cURL : public Thread
{
public:

    //==============================================================================
	enum NetworkOperationMode
	{
		AsyncMode     = 0,
		SyncMode      = 1
	};

    //==============================================================================
	cURL (const NetworkOperationMode& mode = SyncMode);
	~cURL();

    //==============================================================================
	bool initialize();

    //==============================================================================
	bool setOption (const int curlOption, const int curlValue);
	bool setOption (const int curlOption, const String curlValue);

    //==============================================================================
	//bool getInfo (const int curlOption, String &stringValue);
	//bool getInfo (const int curlOption, double &doubleValue);
	//bool getInfo (const int curlOption, long &longValue);

    //==============================================================================
	void setMode (const NetworkOperationMode& shouldBeAsync);
	const NetworkOperationMode getMode();

    //==============================================================================
	bool startTransfer();
	const String getErrorString();

    //==============================================================================
	void addListener (cURLListener *ptr);
	void removeListener (cURLListener *ptr);
	cURLListener* getListener (const int index);
	int getListenersCount () const;
	
    //==============================================================================
	MemoryBlock getRawData();
	String getStringData();
	const uint32 getDataSize();
	
    //==============================================================================
    /** @internal */
	void run();
    /** @internal */
	MemoryBlock *getMemoryBlock();
    /** @internal */
	const uint32 getCurrentPosition();
    /** @internal */
	void setCurrentPosition (const uint32 newPosition);

private:

    //==============================================================================
	void notifyTransferStart ();
	void notifyTransferEnd (const bool errorIndicator);

	CriticalSection cs;
	NetworkOperationMode mode;
	String currentError;

	MemoryBlock *bl;
	uint32 currentPosition;

	VoidArray listeners;

	void* curl;
};

#endif

#endif // __JUCETICE_NET_HEADER__
