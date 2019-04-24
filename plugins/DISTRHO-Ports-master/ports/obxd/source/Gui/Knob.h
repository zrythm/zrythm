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
#include "../Engine/SynthEngine.h"
class Knob  : public Slider
{
public:
	//Knob(Image image, const int numFrames, const bool stripIsHorizontal) : Slider("Knob")
	//{
		
	//	setTextBoxStyle(NoTextBox, 0, 0, 0);
	//	setSliderStyle(RotaryVerticalDrag);
	//	setRange(0.0f, 1.0f, 0.001f);
	//}
	Knob(Image k,int fh) : Slider("Knob")
	{
		h2 =fh;
		w2 = k.getWidth();
		numFr = k.getHeight() / h2;
		kni = k;
	};
	
	void paint(Graphics& g)
	{
		int ofs = (int)((getValue() - getMinimum()) / (getMaximum() - getMinimum()) * (numFr - 1));
				g.drawImage(kni, 0, 0, getWidth(), getHeight(),
					0, h2*ofs, w2, h2);

	}
private:
	Image kni;
	int fh,numFr;
	int w2,h2;
};