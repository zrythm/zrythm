#include "BandLimit.h"
#include <emmintrin.h>
#include <mmintrin.h>

CAllPassFilterPair::CAllPassFilterPair(double coeff_A, double coeff_B)
: a(coeff_A), b(coeff_B), md(5), mf(5)
{
	clear();
};



void CAllPassFilterPair::processBlock(double* data, int numSamples)
{
	jassert((((size_t) data) & 0xF) == 0);
	jassert((_mm_getcsr() & 0x8040) == 0x8040);

	__m128d coeff = _mm_load_pd(md.getPtr(0));
	__m128d x1 = _mm_load_pd(md.getPtr(1));
	__m128d x2 = _mm_load_pd(md.getPtr(2));
	__m128d y1 = _mm_load_pd(md.getPtr(3));
	__m128d y2 = _mm_load_pd(md.getPtr(4));

	for (int i=0; i<numSamples; ++i)
	{
		__m128d x0 = _mm_load_pd(&(data[i+i]));
		__m128d tmp = _mm_sub_pd(x0, y2);
		tmp = _mm_mul_pd(tmp, coeff);
		__m128d y0 = _mm_add_pd(x2, tmp);

		_mm_store_pd(&(data[i+i]), y0);

		x2=x1;
		x1=x0;

		y2=y1;
		y1=y0;
	}

	_mm_store_pd(md.getPtr(1), x1);
	_mm_store_pd(md.getPtr(2), x2);
	_mm_store_pd(md.getPtr(3), y1);
	_mm_store_pd(md.getPtr(4), y2);

};

void CAllPassFilterPair::processBlock(float* data, int numSamples)
{
	jassert((((size_t) data) & 0xF) == 0);
	jassert((_mm_getcsr() & 0x8040) == 0x8040);

	__m128 coeff = _mm_load_ps(mf.getPtr(0));
	__m128 x1 = _mm_load_ps(mf.getPtr(1));
	__m128 x2 = _mm_load_ps(mf.getPtr(2));
	__m128 y1 = _mm_load_ps(mf.getPtr(3));
	__m128 y2 = _mm_load_ps(mf.getPtr(4));

	for (int i=0; i<numSamples; ++i)
	{
		__m128 x0 = _mm_load_ps(&(data[4*i]));
		__m128 tmp = _mm_sub_ps(x0, y2);
		tmp = _mm_mul_ps(tmp, coeff);
		__m128 y0 = _mm_add_ps(x2, tmp);

		_mm_store_ps(&(data[4*i]), y0);

		x2=x1;
		x1=x0;

		y2=y1;
		y1=y0;
	}

	_mm_store_ps(mf.getPtr(1), x1);
	_mm_store_ps(mf.getPtr(2), x2);
	_mm_store_ps(mf.getPtr(3), y1);
	_mm_store_ps(mf.getPtr(4), y2);

};

void CAllPassFilterPair::clear()
{
	md.clear();
	md.set(0, a, b);

	mf.clear();
	mf.set(0, (float) a, (float) a, (float) b, (float) b);
}

// ================================================================================================
CAllPassFilterCascadePair::CAllPassFilterCascadePair(const double* coefficients_A, const double* coefficients_B, const int N)
: numfilters(N)
{
	for (int i=0;i<N;i++)
		allpassfilter.add(new CAllPassFilterPair(coefficients_A[i], coefficients_B[i]));

};

void CAllPassFilterCascadePair::processBlock(double* data, int numSamples)
{
	for (int i=0; i<numfilters; ++i)
		allpassfilter.getUnchecked(i)->processBlock(data, numSamples);
}

void CAllPassFilterCascadePair::processBlock(float* data, int numSamples)
{
	for (int i=0; i<numfilters; ++i)
		allpassfilter.getUnchecked(i)->processBlock(data, numSamples);
}

void CAllPassFilterCascadePair::clear()
{
	for (int i=0; i<numfilters; ++i)
		allpassfilter.getUnchecked(i)->clear();
}

// ===============================================================================================
CHalfBandFilter::CHalfBandFilter(const int order, const bool steep)
: blockSize(0)
{
	if (steep==true)
	{
		if (order==12)	//rejection=104dB, transition band=0.01
		{
			double a_coefficients[6]=
			{0.036681502163648017
			,0.2746317593794541
			,0.56109896978791948
			,0.769741833862266
			,0.8922608180038789
			,0.962094548378084
			};

			double b_coefficients[6]=
			{0.13654762463195771
			,0.42313861743656667
			,0.6775400499741616
			,0.839889624849638
			,0.9315419599631839
			,0.9878163707328971
			};
			filter = new CAllPassFilterCascadePair(a_coefficients, b_coefficients, 6);
	
			//filter_a=new CAllPassFilterCascade(a_coefficients,6);
			//filter_b=new CAllPassFilterCascade(b_coefficients,6);
		}
		else if (order==10)	//rejection=86dB, transition band=0.01
		{
			double a_coefficients[5]=
			{0.051457617441190984
			,0.35978656070567017
			,0.6725475931034693
			,0.8590884928249939
			,0.9540209867860787
			};

			double b_coefficients[5]=
			{0.18621906251989334
			,0.529951372847964
			,0.7810257527489514
			,0.9141815687605308
			,0.985475023014907
			};

			filter = new CAllPassFilterCascadePair(a_coefficients, b_coefficients, 5);
	
			//filter_a=new CAllPassFilterCascade(a_coefficients,5);
			//filter_b=new CAllPassFilterCascade(b_coefficients,5);
		}
		else if (order==8)	//rejection=69dB, transition band=0.01
		{
			double a_coefficients[4]=
			{0.07711507983241622
			,0.4820706250610472
			,0.7968204713315797
			,0.9412514277740471
			};

			double b_coefficients[4]=
			{0.2659685265210946
			,0.6651041532634957
			,0.8841015085506159
			,0.9820054141886075
			};
	
			filter = new CAllPassFilterCascadePair(a_coefficients, b_coefficients, 4);	

			//filter_a=new CAllPassFilterCascade(a_coefficients,4);
			//filter_b=new CAllPassFilterCascade(b_coefficients,4);
		}
		else if (order==6)	//rejection=51dB, transition band=0.01
		{
			double a_coefficients[3]=
			{0.1271414136264853
			,0.6528245886369117
			,0.9176942834328115
			};

			double b_coefficients[3]=
			{0.40056789819445626
			,0.8204163891923343
			,0.9763114515836773
			};
	
			filter = new CAllPassFilterCascadePair(a_coefficients, b_coefficients, 3);
	
			//filter_a=new CAllPassFilterCascade(a_coefficients,3);
			//filter_b=new CAllPassFilterCascade(b_coefficients,3);
		}
		else if (order==4)	//rejection=53dB,transition band=0.05
		{
			double a_coefficients[2]=
			{0.12073211751675449
			,0.6632020224193995
			};

			double b_coefficients[2]=
			{0.3903621872345006
			,0.890786832653497
			};
	
			filter = new CAllPassFilterCascadePair(a_coefficients, b_coefficients, 2);
	
			//filter_a=new CAllPassFilterCascade(a_coefficients,2);
			//filter_b=new CAllPassFilterCascade(b_coefficients,2);
		}
	
		else	//order=2, rejection=36dB, transition band=0.1
		{
			double a_coefficients=0.23647102099689224;
			double b_coefficients=0.7145421497126001;

			filter = new CAllPassFilterCascadePair(&a_coefficients, &b_coefficients, 1);

			//filter_a=new CAllPassFilterCascade(&a_coefficients,1);
			//filter_b=new CAllPassFilterCascade(&b_coefficients,1);
		}
	}
	else	//softer slopes, more attenuation and less stopband ripple
	{
		if (order==12)	//rejection=150dB, transition band=0.05
		{
			double a_coefficients[6]=
			{0.01677466677723562
			,0.13902148819717805
			,0.3325011117394731
			,0.53766105314488
			,0.7214184024215805
			,0.8821858402078155
			};

			double b_coefficients[6]=
			{0.06501319274445962
			,0.23094129990840923
			,0.4364942348420355
			,0.06329609551399348
			,0.80378086794111226
			,0.9599687404800694
			};
	
			filter = new CAllPassFilterCascadePair(a_coefficients, b_coefficients, 6);
	
			//filter_a=new CAllPassFilterCascade(a_coefficients,6);
			//filter_b=new CAllPassFilterCascade(b_coefficients,6);
		}
		else if (order==10)	//rejection=133dB, transition band=0.05
		{
			double a_coefficients[5]=
			{0.02366831419883467
			,0.18989476227180174
			,0.43157318062118555
			,0.6632020224193995
			,0.860015542499582
			};

			double b_coefficients[5]=
			{0.09056555904993387
			,0.3078575723749043
			,0.5516782402507934
			,0.7652146863779808
			,0.95247728378667541
			};
	
			filter = new CAllPassFilterCascadePair(a_coefficients, b_coefficients, 5);

			//filter_a=new CAllPassFilterCascade(a_coefficients,5);
			//filter_b=new CAllPassFilterCascade(b_coefficients,5);
		}
		else if (order==8)	//rejection=106dB, transition band=0.05
		{
			double a_coefficients[4]=
			{0.03583278843106211
			,0.2720401433964576
			,0.5720571972357003
			,0.827124761997324
			};

			double b_coefficients[4]=
			{0.1340901419430669
			,0.4243248712718685
			,0.7062921421386394
			,0.9415030941737551
			};
	
			filter = new CAllPassFilterCascadePair(a_coefficients, b_coefficients, 4);

			//filter_a=new CAllPassFilterCascade(a_coefficients,4);
			//filter_b=new CAllPassFilterCascade(b_coefficients,4);
		}
		else if (order==6)	//rejection=80dB, transition band=0.05
		{
			double a_coefficients[3]=
			{0.06029739095712437
			,0.4125907203610563
			,0.7727156537429234
			};

			double b_coefficients[3]=
			{0.21597144456092948
			,0.6043586264658363
			,0.9238861386532906
			};
	
			filter = new CAllPassFilterCascadePair(a_coefficients, b_coefficients, 3);
	
			//filter_a=new CAllPassFilterCascade(a_coefficients,3);
			//filter_b=new CAllPassFilterCascade(b_coefficients,3);
		}
		else if (order==4)	//rejection=70dB,transition band=0.1
		{
			double a_coefficients[2]=
			{0.07986642623635751
			,0.5453536510711322
			};

			double b_coefficients[2]=
			{0.28382934487410993
			,0.8344118914807379
			};
	
			filter = new CAllPassFilterCascadePair(a_coefficients, b_coefficients, 2);

			//filter_a=new CAllPassFilterCascade(a_coefficients,2);
			//filter_b=new CAllPassFilterCascade(b_coefficients,2);
		}
	
		else	//order=2, rejection=36dB, transition band=0.1
		{
			double a_coefficients=0.23647102099689224;
			double b_coefficients=0.7145421497126001;

			filter = new CAllPassFilterCascadePair(&a_coefficients, &b_coefficients, 1);

			//filter_a=new CAllPassFilterCascade(&a_coefficients,1);
			//filter_b=new CAllPassFilterCascade(&b_coefficients,1);
		}
	}

	clear();
};

void CHalfBandFilter::clear()
{
	filter->clear();
	oldout = 0;
	oldOutL = 0;
	oldOutR = 0;
}

void CHalfBandFilter::setBlockSize(int newBlockSize)
{
	if (newBlockSize > blockSize)
	{
		blockSize = newBlockSize;
		bufferDouble.setSize(blockSize);
		bufferFloat.setSize(blockSize);
	}
}

void CHalfBandFilter::processBlock(float* data, int numSamples)
{
	setBlockSize(numSamples);

	double* proc = bufferDouble.getPtr(0);

	{
		int n=0;
		for (int i=0; i<numSamples; ++i)
		{
			const float x = data[i];
			proc[n++] = x;
			proc[n++] = x;
		}
	}

	filter->processBlock(proc, numSamples);

	{
		int n=0;
		for (int i=0; i<numSamples; ++i)
		{
			const float output = float(proc[n++] + oldout) * 0.5f;
			oldout = proc[n++];

			data[i] = output;
		}
	}
}

void CHalfBandFilter::processBlock(float* dataL, float* dataR, int numSamples)
{
	setBlockSize(numSamples);

	float* proc = bufferFloat.getPtr(0);

	{
		int n=0;
		for (int i=0; i<numSamples; ++i)
		{
			const float l = dataL[i];
			const float r = dataR[i];
			proc[n++] = l;
			proc[n++] = l;
			proc[n++] = r;
			proc[n++] = r;
		}
	}

	filter->processBlock(proc, numSamples);

	{
		int n=0;
		for (int i=0; i<numSamples; ++i)
		{
			const float outputL = float(proc[n++] + oldOutL) * 0.5f;
			const float outputR = float(proc[n++] + oldOutR) * 0.5f;

			oldOutL = proc[n++];
			oldOutR = proc[n++];

			dataL[i] = outputL;
			dataR[i] = outputR;
		}
	}
}


// ================================================================================================
UpSampler2x::UpSampler2x(int order, bool steep)
: filter(order, steep)
{
}

void UpSampler2x::processBlock(const float* in, float* out, int numInSamples)
{
	int n=0;

	for (int i=0; i<numInSamples; ++i)
	{
		out[n++] = in[i] * 2.f;
		out[n++] = 0;		
	}

	filter.processBlock(out, 2*numInSamples);
}

void UpSampler2x::processBlock(const float* inL, const float* inR, float* outL, float* outR, int numInSamples)
{
	int n=0;

	for (int i=0; i<numInSamples; ++i)
	{
		outL[n] = inL[i] * 2.f;
		outR[n++] = inR[i] * 2.f;
		outL[n] = 0;		
		outR[n++] = 0;		
	}

	filter.processBlock(outL, outR, 2*numInSamples);
}

void UpSampler2x::clear()
{
	filter.clear();
}



DownSampler2x::DownSampler2x(int order, bool steep)
: filter(order, steep)
{
}

// ================================================================================================
void DownSampler2x::processBlock(float* in, float* out, int numOutSamples)
{
	filter.processBlock(in, 2*numOutSamples);

	int n=0;

	for (int i=0; i<numOutSamples; ++i)
	{
		out[i] = in[n];
		n += 2;
	}
}
void DownSampler2x::processBlock(float* inL, float* inR, float* outL, float* outR, int numOutSamples)
{
	filter.processBlock(inL, inR, 2*numOutSamples);

	int n=0;

	for (int i=0; i<numOutSamples; ++i)
	{
		outL[i] = inL[n];
		outR[i] = inR[n];
		n += 2;
	}
}

void DownSampler2x::clear()
{
	filter.clear();
}

// ================================================================================================
OverSampler2x::OverSampler2x(int order, bool steep)
: upSampler(order, steep),
	downSampler(order, steep),
	blockSize(0)
{
}

void OverSampler2x::setBlockSize(int newBlockSize)
{
	if (newBlockSize > blockSize)
	{
		blockSize = newBlockSize;
		data.realloc(blockSize);
		dataAux.realloc(blockSize);

	}
}

float* OverSampler2x::processUp(float* in, int numSamples)
{
	setBlockSize(2*numSamples);

	upSampler.processBlock(in, data, numSamples);

	return data;
}

float** OverSampler2x::processUp(float* inL, float* inR, int numSamples)
{
	setBlockSize(2*numSamples);

	upSampler.processBlock(inL, inR, data, dataAux, numSamples);

	stereoData[0] = data;
	stereoData[1] = dataAux;
	return stereoData;
}

void OverSampler2x::processDown(float* in, float* out, int numSamples)
{
	setBlockSize(2*numSamples);

	downSampler.processBlock(in, out, numSamples);
}


void OverSampler2x::processDown(float* inL, float* inR, float* outL, float* outR, int numSamples)
{
	setBlockSize(2*numSamples);

	downSampler.processBlock(inL, inR, outL, outR, numSamples);
}


void OverSampler2x::clear()
{
	upSampler.clear();
	downSampler.clear();
}