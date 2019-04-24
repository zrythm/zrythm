/*
	==============================================================================
	This file is part of Obxd synthesizer.

	Copyright © 2013-2014 Filatov Vadim
	
	Contact author via email :
	justdat_@_e1.ru

	This file may be licensed under the terms of of the
	GNU General Public License Version 2 (the ``GPL'').

	Software distributed under the License is distributed
	on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
	express or implied. See the GPL for the specific language
	governing rights and limitations.

	You should have received a copy of the GPL along with this
	program. If not, go to http://www.gnu.org/licenses/gpl.html
	or write to the Free Software Foundation, Inc.,  
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
	==============================================================================
 */
#pragma once
#include "SynthEngine.h"
#include "BlepData.h"
class TriangleOsc 
{
	DelayLine<Samples> del1;
	bool fall;
	float buffer1[Samples*2];
	const int hsam;
	const int n;
	float const * blepPTR;
	float const * blampPTR;

	int bP1,bP2;
public:
	TriangleOsc() : hsam(Samples)
		, n(Samples*2)
	{
		//del1 =new DelayLine(hsam);
		fall = false;
		bP1=bP2=0;
	//	buffer1= new float[n];
		for(int i = 0 ; i < n ; i++)
			buffer1[i]=0;
		blepPTR = blep;
		blampPTR = blamp;
	}
	~TriangleOsc()
	{
		//delete buffer1;
		//delete del1;
	}
	inline void setDecimation()
	{
		blepPTR = blepd2;
		blampPTR = blampd2;
	}
	inline void removeDecimation()
	{
		blepPTR = blep;
		blampPTR = blamp;
	}
	inline float aliasReduction()
	{
		return -getNextBlep(buffer1,bP1);
	}
	inline void processMaster(float x,float delta)
	{
		if(x >= 1.0)
		{
			x-=1.0;
			mixInBlampCenter(buffer1,bP1,x/delta,-4*Samples*delta);
		}
		if(x >= 0.5 && x - delta < 0.5)
		{
			mixInBlampCenter(buffer1,bP1,(x-0.5)/delta,4*Samples*delta);
		}
		if(x >= 1.0)
		{
			x-=1.0;
			mixInBlampCenter(buffer1,bP1,x/delta,-4*Samples*delta);
		}
	}
	inline float getValue(float x)
	{
		float mix = x < 0.5 ? 2*x-0.5 : 1.5-2*x;
		return del1.feedReturn(mix);
	}
	inline float getValueFast(float x)
	{
		float mix = x < 0.5 ? 2*x-0.5 : 1.5-2*x;
		return mix;
	}
	inline void processSlave(float x , float delta,bool hardSyncReset,float hardSyncFrac)
	{
		bool hspass = true;
		if(x >= 1.0)
		{
			x-=1.0;
			if(((!hardSyncReset)||(x/delta > hardSyncFrac)))//de morgan processed equation
			{
				mixInBlampCenter(buffer1,bP1,x/delta,-4*Samples*delta);
			}
			else
			{
				x+=1;
				hspass = false;
			}
		}
		if(x >= 0.5 && x - delta < 0.5 &&hspass)
		{
			float frac = (x - 0.5) / delta;
			if(((!hardSyncReset)||(frac > hardSyncFrac)))//de morgan processed equation
			{
				mixInBlampCenter(buffer1,bP1,frac,4*Samples*delta);
			}
		}
		if(x >= 1.0 && hspass)
		{
			x-=1.0;
			if(((!hardSyncReset)||(x/delta > hardSyncFrac)))//de morgan processed equation
			{
				mixInBlampCenter(buffer1,bP1,x/delta,-4*Samples*delta);
			}
			else
			{
				//if transition do not ocurred 
				x+=1;
			}
		}
		if(hardSyncReset)
		{
			float fracMaster = (delta * hardSyncFrac);
			float trans = (x-fracMaster);
			float mix = trans < 0.5 ? 2*trans-0.5 : 1.5-2*trans;
			if(trans >0.5)
				mixInBlampCenter(buffer1,bP1,hardSyncFrac,-4*Samples*delta);
			mixInImpulseCenter(buffer1,bP1,hardSyncFrac,mix+0.5);
		}
	}
	inline void mixInBlampCenter(float * buf,int& bpos,float offset, float scale)
	{
		int lpIn =(int)(B_OVERSAMPLING*(offset));
		float frac = offset * B_OVERSAMPLING - lpIn;
		float f1 = 1.0f-frac;
		for(int i = 0 ; i < n;i++)
		{
			float mixvalue = (blampPTR[lpIn]*f1+blampPTR[lpIn+1]*(frac));
			buf[(bpos+i)&(n-1)]  += mixvalue*scale;
			lpIn += B_OVERSAMPLING;
		}
	}
		inline void mixInImpulseCenter(float * buf,int& bpos,float offset, float scale) 
	{
		int lpIn =(int)(B_OVERSAMPLING*(offset));
		float frac = offset * B_OVERSAMPLING - lpIn;
		float f1 = 1.0f-frac;
		for(int i = 0 ; i < Samples;i++)
		{
			float mixvalue = (blepPTR[lpIn]*f1+blepPTR[lpIn+1]*(frac));
			buf[(bpos+i)&(n-1)]  += mixvalue*scale;
			lpIn += B_OVERSAMPLING;
		}
		for(int i = Samples ; i <n;i++)
		{
			float mixvalue = (blepPTR[lpIn]*f1+blepPTR[lpIn+1]*(frac));
			buf[(bpos+i)&(n-1)]  -= mixvalue*scale;
			lpIn += B_OVERSAMPLING;
		}
	}
	inline float getNextBlep(float* buf,int& bpos) 
	{
		buf[bpos]= 0.0f;
		bpos++;

		// Wrap pos
		bpos&=(n-1);
		return buf[bpos];
	}
};
