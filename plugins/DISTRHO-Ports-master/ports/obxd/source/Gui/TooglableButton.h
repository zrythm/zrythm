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
class TooglableButton : public ImageButton
{
public:
	bool toogled;
	TooglableButton(Image k) :ImageButton()
	{
		//this->setImages
		kni = k;
		toogled = false;
		width = kni.getWidth();
		height = kni.getHeight();
		w2=width;
		h2 = height / 2;
		this->setClickingTogglesState(true);
	}
	void clicked()
	{
		toogled = !toogled;
		//this->setColour(1,Colours::blue);
		//if(toogled)
		//	this->setColour(TextButton::ColourIds::buttonColourId,Colours::lightgreen);
		//else
		//	this->removeColour(TextButton::ColourIds::buttonColourId);
		//this->setColour(DrawableButton::ColourIds::backgroundColourId,Colours::lightpink);
		Button::clicked();

	};
	void paintButton(Graphics& g, bool isMouseOverButton, bool isButtonDown)
	{
		        int offset = 0;
        if (toogled)
        {
            offset = 1;
        }
		g.drawImage(kni, 0, 0, getWidth(), getHeight(),
				0, offset *h2, w2,h2);
	}
	void setValue(float state,int notify)
	{
		if(state > 0.5)
			toogled = true;
		else toogled = false;
		repaint();
	}
	float getValue()
	{
		if(toogled)
			return 1;
		else return 0;
	}
	//void paint(Graphics& g)
	//{
	//	g.drawImageTransformed(kni,AffineTransform::rotation(((getValue() - getMinimum())/(getMaximum() - getMinimum()))*float_Pi - float_Pi*2));
	//}
private:
	Image kni;
	int width,height,w2,h2;
};