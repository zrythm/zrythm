#include "JuceHeader.h"

class CAllPassFilterPair
{
public:

	struct AlignedDouble
	{
		AlignedDouble(int size_ = 0)
			: size(0)
		{
			setSize(size_);
		}

		void setSize(int newSize)
		{
			if (newSize != size)
			{
				size = newSize;
				data.realloc(size*2 + 2);
				const size_t addr = (size_t) (double*) data;
				const size_t offset = addr & 0xF;
				ptr = (double*) (addr + 0x10 - offset);
			}
		}

		double* getPtr(int idx) { return &(ptr[idx+idx]); }

		void set(int idx, double value1, double value2)
		{
			ptr[idx+idx] = value1;
			ptr[idx+idx+1] = value2;
		}

		const int getSize() { return size; }

		void clear()
		{
			for (int i=0; i<size*2; ++i)
				ptr[i] = 0;
		}

	private:
		double* ptr;
		HeapBlock<double> data;
		int size;
	};

	struct AlignedFloat
	{
		AlignedFloat(int size_ = 0)
			: size(0)
		{
			setSize(size_);
		}

		void setSize(int newSize)
		{
			if (newSize != size)
			{
				size = newSize;
				data.realloc(size*4 + 4);
				const size_t addr = (size_t) (float*) data;
				const size_t offset = addr & 0xF;
				ptr = (float*) (addr + 0x10 - offset);
			}
		}

		float* getPtr(int idx) { return &(ptr[4*idx]); }

		void set(int idx, float value1, float value2, float value3, float value4)
		{
			ptr[4*idx] = value1;
			ptr[4*idx+1] = value2;
			ptr[4*idx+2] = value3;
			ptr[4*idx+3] = value4;
		}

		const int getSize() { return size; }

		void clear()
		{
			for (int i=0; i<size*4; ++i)
				ptr[i] = 0;
		}

	private:
		float* ptr;
		HeapBlock<float> data;
		int size;
	};

	CAllPassFilterPair(double coeff_A, double coeff_B);

	void processBlock(double* data, int numSamples);
	void processBlock(float* data, int numSamples);

	void clear();

private:
	double a;
	double b;

	AlignedDouble md;
	AlignedFloat mf;
};


class CAllPassFilterCascadePair
{
public:
	CAllPassFilterCascadePair(const double* coefficients_A, const double* coefficients_B, int N);

	void processBlock(double* data, int numSamples);
	void processBlock(float* data, int numSamples);

	void clear();

private:
	OwnedArray<CAllPassFilterPair> allpassfilter;
	int numfilters;
};


class CHalfBandFilter
{
public:
	CHalfBandFilter(const int order, const bool steep);

	void setBlockSize(int newBlockSize);
	void processBlock(float* data, int numSamples);
	void processBlock(float* dataL, float* dataR, int numSamples);

	void clear();

private:
	ScopedPointer<CAllPassFilterCascadePair> filter;

	double oldout;
	float oldOutL;
	float oldOutR;

	int blockSize;
	CAllPassFilterPair::AlignedDouble bufferDouble;
	CAllPassFilterPair::AlignedFloat bufferFloat;

};




class UpSampler2x
{
public:
	UpSampler2x(int order, bool steep);

	void processBlock(const float* in, float* out, int numInSamples);
	void processBlock(const float* inL, const float* inR, float* outL, float* outR, int numInSamples);

	void clear();


private:

	CHalfBandFilter filter;
};


class DownSampler2x
{
public:
	DownSampler2x(int order, bool steep);

	void processBlock(float* in, float* out, int numOutSamples);
	void processBlock(float* inL, float* inR, float* outL, float* outR, int numOutSamples);

	void clear();


private:

	CHalfBandFilter filter;
};



class OverSampler2x
{
public:
	OverSampler2x(int order, bool steep);

	void setBlockSize(int newBlockSize);

	float* processUp(float* in, int numSamples);
	float** processUp(float* inL, float* inR, int numSamples);

	void processDown(float* in, float* out, int numSamples);
	void processDown(float* inL, float* inR, float* outL, float* outR, int numSamples);

	void clear();

private:

	UpSampler2x upSampler;
	DownSampler2x downSampler;

	HeapBlock<float> data;
	HeapBlock<float> dataAux;

	float* stereoData[2];
	int blockSize;
};






