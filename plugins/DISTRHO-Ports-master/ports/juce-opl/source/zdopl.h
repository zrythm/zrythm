#ifndef OPL_H
#define OPL_H

// Abstract base class for OPL emulators

class OPLEmul
{
public:
	OPLEmul() {}
	virtual ~OPLEmul() {}

	virtual void Reset() = 0;
	virtual void WriteReg(int reg, int v) = 0;
	virtual void Update(float *buffer, int length) = 0;
	virtual void SetPanning(int c, float left, float right) = 0;
};

OPLEmul *JavaOPLCreate(bool stereo);

#define OPL_SAMPLE_RATE			49716.0
#define CENTER_PANNING_POWER	0.70710678118	/* [RH] volume at center for EQP */


#endif
