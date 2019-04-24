/*
	==============================================================================
	This file is part of Tal-Reverb by Patrick Kunz.

	Copyright(c) 2005-2010 Patrick Kunz, TAL
	Togu Audio Line, Inc.
	http://kunz.corrupt.ch

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

#ifndef AccordeonTab_H
#define AccordeonTab_H

#include "../TalComponent.h"

class AccordeonTab : public Button
{
private:
    int tabHeight;

    Image image;
    int startY;
    int maxHeight;

public:
    AccordeonTab(Image image, int startY, int maxHeight) : Button("asdf")
	{
       this->image = image;
       this->startY = startY;
       this->maxHeight = maxHeight;
       this->tabHeight = 25;

       //setBounds(0, startY, image.getWidth(), maxHeight);
       this->setBufferedToImage(true);
	}

    ~AccordeonTab() override
    {
        deleteAllChildren();
    }

    void setTabY(int y, bool refresh)
    {
        if (y != this->getY() || getTotalHeight() != getWidth() || refresh)
        {
            setBounds(0, y, image.getWidth(), getTotalHeight());
        }
    }

    void paintButton(Graphics& g, bool isMouseOverButton, bool isButtonDown) override
    {
        g.drawImage (image,
                     0, 0, image.getWidth(), this->getTotalHeight(),
                     0, startY, image.getWidth(), this->getTotalHeight());
    }

    int getTabHeight()
    {
        return this->tabHeight;
    }

    int getMaxHeight()
    {
        return this->maxHeight;
    }

    int getTotalHeight()
    {
        if (this->isExpanded())
        {
            return  this->maxHeight;
        }
        else
        {
            return this->tabHeight;
        }
    }

    bool isExpanded()
    {
        return getToggleState();
    }

    void mouseDown(const MouseEvent &e) override
    {
        if (e.getPosition().getY() < this->tabHeight)
        {
            setToggleState(!getToggleState(), sendNotification);
        }
    }

    void setExpanded(bool expanded)
    {
        setToggleState(expanded, sendNotification);
    }
};
#endif
