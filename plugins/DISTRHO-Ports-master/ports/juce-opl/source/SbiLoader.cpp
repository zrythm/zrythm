#include "SbiLoader.h"


SbiLoader::SbiLoader(void)
{
}

void SbiLoader::loadInstrumentData(int n, const unsigned char* data, AdlibBlasterAudioProcessor *proc)
{
	if (0 == strncmp("SBI", (const char*)data, 3)) {
		data += 36;
		proc->setParametersByRegister(0x20, 0, data[0]);
		proc->setParametersByRegister(0x20, 1, data[1]);
		proc->setParametersByRegister(0x40, 0, data[2]);
		proc->setParametersByRegister(0x40, 1, data[3]);
		proc->setParametersByRegister(0x60, 0, data[4]);
		proc->setParametersByRegister(0x60, 1, data[5]);
		proc->setParametersByRegister(0x80, 0, data[6]);
		proc->setParametersByRegister(0x80, 1, data[7]);
		proc->setParametersByRegister(0xE0, 0, data[8]);
		proc->setParametersByRegister(0xE0, 1, data[9]);
		proc->setParametersByRegister(0xC0, 1, data[10]);
	} // else throw "Invalid header";
}

String SbiLoader::getExtension()
{
	return String("sbi");
}

SbiLoader::~SbiLoader(void)
{
}

