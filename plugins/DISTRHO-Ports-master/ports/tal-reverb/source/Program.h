#ifndef ProgramChunk_H
#define ProgramChunk_H

/*-----------------------------------------------------------------------------

Kunz Patrick 15.08.2007

Default Presets saved as Hex values. No need for system dependent resource
including.

-----------------------------------------------------------------------------*/

const unsigned char chunk[]= {
};

class ProgramChunk
{

public:
	ProgramChunk()  
	{
	}

	const unsigned char* getChunk() 
	{
		return chunk;
	}
};
#endif