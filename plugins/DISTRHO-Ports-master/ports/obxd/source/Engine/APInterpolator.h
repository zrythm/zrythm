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
class ApInterpolator
{
	private:
	const float Nu;
	float zd;
	float li;
public :
	ApInterpolator() : Nu(( 1 - 0.5) /(1 + 0.5))
		{
			zd = 0;
			li=0;
	};
	inline float getInterp(float in)
	{
		float out = Nu * ( in - zd) + li;
		zd = out;
		li = in;
		return out;
	}
};