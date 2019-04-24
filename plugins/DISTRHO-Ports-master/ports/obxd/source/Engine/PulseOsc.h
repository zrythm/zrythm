/*
	==============================================================================
	This file is part of Obxd synthesizer.

	Copyright � 2013-2014 Filatov Vadim
	
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
class PulseOsc 
{
	DelayLine<Samples> del1;
	bool pw1t;
	float buffer1[Samples*2];
	const int hsam;
	const int n;
	float const * blepPTR;
	int bP1;
public:
	PulseOsc() : hsam(Samples)
		, n(Samples*2)
	{
	//	del1 = new DelayLine(hsam);
		pw1t = false;
		bP1=0;
		//buffer1= new float[n];
		for(int i = 0 ; i < n ; i++)
			buffer1[i]=0;
		blepPTR = blep;
	}
	~PulseOsc()
	{
	//	delete buffer1;
	//	delete del1;
	}
	inline void setDecimation()
	{
		blepPTR = blepd2;
	}
	inline void removeDecimation()
	{
		blepPTR = blep;
	}
	inline float aliasReduction()
	{
		return -getNextBlep(buffer1,bP1);
	}
	inline void processMaster(float x,float delta,float pulseWidth,float pulseWidthWas)
	{
		float summated = delta- (pulseWidth - pulseWidthWas);
		if((pw1t) && x >= 1.0f)
		{
			x -= 1.0f;
			if(pw1t)
				mixInImpulseCenter(buffer1,bP1,x/delta, 1);
			pw1t=false;
		}
		if((!pw1t)&& (x >= pulseWidth)&&(x - summated <=pulseWidth))
		{
			pw1t=true;
			float frac  =(x-pulseWidth) / summated;
			mixInImpulseCenter(buffer1,bP1,frac,-1);
		}
		if((pw1t) && x >= 1.0f)
		{
			x-=1.0f;
			if(pw1t)
				mixInImpulseCenter(buffer1,bP1,x/delta, 1);
			pw1t=false;
		}

	}
	inline float getValue(float x,float pulseWidth)
	{
		float oscmix;
		if(x >= pulseWidth)
			oscmix = 1 - (0.5-pulseWidth) - 0.5;
		else
			oscmix = -(0.5-pulseWidth) - 0.5;
		return del1.feedReturn(oscmix);
	}
	inline float getValueFast(float x,float pulseWidth)
	{
		float oscmix;
		if(x >= pulseWidth)
			oscmix = 1 - (0.5-pulseWidth) - 0.5;
		else
			oscmix = -(0.5-pulseWidth) - 0.5;
		return oscmix;
	}
	inline void processSlave(float x , float delta,bool hardSyncReset,float hardSyncFrac,float pulseWidth,float pulseWidthWas)
	{
		float summated = delta- (pulseWidth - pulseWidthWas);

		if((pw1t) && x >= 1.0f)
		{
			x -= 1.0f;
			if(((!hardSyncReset)||(x/delta > hardSyncFrac)))//de morgan processed equation
			{
				if(pw1t)
					mixInImpulseCenter(buffer1,bP1,x/delta, 1);
				pw1t=false;
			}
			else
			{
				x+=1;
			}
		}

		if((!pw1t)&& (x >= pulseWidth) && (x - summated <=pulseWidth))
		{
			pw1t=true;
			float frac  =(x-pulseWidth) / summated;
			if(((!hardSyncReset)||(frac > hardSyncFrac)))//de morgan processed equation
			{
				//transition to 1
				mixInImpulseCenter(buffer1,bP1,frac,-1);
			}
			else
			{
				//if transition do not ocurred 
				pw1t=false;
			}

		}
		if((pw1t) && x >= 1.0f)
		{
			x -= 1.0f;
			if(((!hardSyncReset)||(x/delta > hardSyncFrac)))//de morgan processed equation
			{
				if(pw1t)
					mixInImpulseCenter(buffer1,bP1,x/delta, 1);
				pw1t=false;
			}
			else
			{
				x+=1;
			}
		}

		if(hardSyncReset)
		{
			//float fracMaster = (delta * hardSyncFrac);
			float trans =(pw1t?1:0);
			mixInImpulseCenter(buffer1,bP1,hardSyncFrac,trans);
			pw1t = false;
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
