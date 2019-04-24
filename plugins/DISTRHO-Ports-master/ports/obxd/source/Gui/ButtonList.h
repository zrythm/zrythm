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
#include "../Engine/SynthEngine.h"
class ButtonList  : public ComboBox{
private :
	int count;
	Image kni;
	int w2,h2;
public:
	ButtonList(Image k,int fh) :ComboBox("cb")
	{
		kni = k;
		count = 0;
		h2 =fh;
		w2 = k.getWidth();
	}
	//int addItem
	void addChoise(String name)
	{
		addItem(name,++count);
	}
	float getValue()
	{
		return ((getSelectedId()-1)/ (float)(count-1));
	}
	void setValue(float val,NotificationType notify)
	{
		setSelectedId((int)(val*(count -1) + 1),notify);
	}
	void paintOverChildren(Graphics& g)
	{
		int ofs = getSelectedId()-1;
				g.drawImage(kni, 0, 0, getWidth(), getHeight(),
					0, h2*ofs, w2, h2);
	}


};
