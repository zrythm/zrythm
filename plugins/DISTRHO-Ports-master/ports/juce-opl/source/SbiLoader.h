#pragma once
#include "InstrumentLoader.h"
class SbiLoader :
	public InstrumentLoader
{
public:
	SbiLoader(void);
	virtual ~SbiLoader(void);

	void loadInstrumentData(int n, const unsigned char* data, AdlibBlasterAudioProcessor *proc);
	String getExtension();
};

