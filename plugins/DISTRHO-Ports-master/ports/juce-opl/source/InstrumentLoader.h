#pragma once
#include "PluginProcessor.h"

// Just specifies the interfaces for instrument loaders.

class InstrumentLoader
{
public:
	virtual void loadInstrumentData(int n, const unsigned char* data, AdlibBlasterAudioProcessor *proc) = 0;
	virtual String getExtension() = 0;
};
