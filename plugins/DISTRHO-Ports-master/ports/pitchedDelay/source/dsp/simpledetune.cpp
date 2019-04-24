#include "simpledetune.h"

DetunerBase::DetunerBase(int bufmax)
	: bufMax(bufmax),
	  windowSize(0),
	  sampleRate(44100),
	  buf(bufMax),
	  win(bufMax)

{
	clear();
	setWindowSize(bufMax);
}

void DetunerBase::clear()
{
	pos0 = 0; 
	pos1 = 0.f;
	pos2 = 0.f;

	for(int i=0;i<bufMax;i++) 
	  buf[i] = 0;
}

void DetunerBase::setWindowSize(int size, bool force)
{
	//recalculate crossfade window
	if (windowSize != size || force)
	{
		windowSize = size;

		if (windowSize >= bufMax) 
			windowSize = bufMax;

		//bufres = 1000.0f * (float)windowSize / sampleRate;

		double p=0.0;
		double dp = 6.28318530718 / windowSize;

		for(int i=0;i<windowSize;i++) 
		{ 
		  win[i] = (float)(0.5 - 0.5 * cos(p)); 
		  p+=dp; 
		}
	}
}

void DetunerBase::setPitchSemitones(float newPitch)
{
	dpos2 = (float)pow(1.0594631f, newPitch);
	dpos1 = 1.0f / dpos2;
}

void DetunerBase::setPitch(float newPitch)
{
	dpos2 = newPitch;
	dpos1 = 1.0f / dpos2;
}

void DetunerBase::setSampleRate(float newSampleRate)
{
	if (sampleRate != newSampleRate)
	{
		sampleRate = newSampleRate;
		setWindowSize(windowSize, true);
	}
}

void DetunerBase::processBlock(float* data, int numSamples)
{
	float a, b, c, d;
	float x;
	float p1 = pos1;
	float p1f;
	float d1 = dpos1;
	float p2 = pos2;
	float d2 = dpos2;
	int  p0 = pos0;
	int p1i;
	int p2i;
	int  l = windowSize-1;
	int lh = windowSize>>1;
	float lf = (float) windowSize;

	for (int i=0; i<numSamples; ++i)
	{
		a = data[i];
		b = a;

		c = 0;
		d = 0;

		--p0 &= l;
		*(buf + p0) = (a);      //input

		p1 -= d1;

		if(p1<0.0f) 
			p1 += lf;           //output

		p1i = (int) p1;
		p1f = p1 - (float) p1i;
		a = *(buf + p1i);
		++p1i &= l;
		a += p1f * (*(buf + p1i) - a);  //linear interpolation

		p2i = (p1i + lh) & l;           //180-degree ouptut
		b = *(buf + p2i);
		++p2i &= l;
		b += p1f * (*(buf + p2i) - b);  //linear interpolation

		p2i = (p1i - p0) & l;           //crossfade
		x = *(win + p2i);
		//++p2i &= l;
		//x += p1f * (*(win + p2i) - x); //linear interpolation (doesn't do much)
		c += b + x * (a - b);

		p2 -= d2;  //repeat for downwards shift - can't see a more efficient way?
		if(p2<0.0f) p2 += lf;           //output
		p1i = (int) p2;
		p1f = p2 - (float) p1i;
		a = *(buf + p1i);
		++p1i &= l;
		a += p1f * (*(buf + p1i) - a);  //linear interpolation

		p2i = (p1i + lh) & l;           //180-degree ouptut
		b = *(buf + p2i);
		++p2i &= l;
		b += p1f * (*(buf + p2i) - b);  //linear interpolation

		p2i = (p1i - p0) & l;           //crossfade
		x = *(win + p2i);
		//++p2i &= l;
		//x += p1f * (*(win + p2i) - x); //linear interpolation (doesn't do much)
		d += b + x * (a - b);

		data[i] = d;
	}

	pos0=p0; 
	pos1=p1; 
	pos2=p2;
}

int DetunerBase::getWindowSize()
{
	return windowSize;
}	
// ==============================================================================================================
Detune::Detune(const String& name_, int windowSize)
: name(name_),
  tunerL(windowSize),
  tunerR(windowSize)
{

}

void Detune::prepareToPlay(double sampleRate, int /*blockSize*/)
{
	tunerL.setSampleRate((float) sampleRate);
	tunerR.setSampleRate((float) sampleRate);
}

void Detune::processBlock(float* chL, float* chR, int numSamples)
{
	tunerL.processBlock(chL, numSamples);
	tunerR.processBlock(chR, numSamples);
}

void Detune::processBlock(float* ch, int numSamples)
{
	tunerL.processBlock(ch, numSamples);
}

void Detune::setPitch(float newPitch)
{
	tunerL.setPitch(newPitch);
	tunerR.setPitch(newPitch);
}

int Detune::getLatency()
{
	return tunerL.getWindowSize();
}

void Detune::clear()
{
	tunerL.clear();
	tunerR.clear();
}

String Detune::getName()
{
	return name;
}
