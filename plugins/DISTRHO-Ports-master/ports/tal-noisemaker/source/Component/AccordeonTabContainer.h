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

#ifndef AccordeonTabContainer_H
#define AccordeonTabContainer_H

#include "AccordeonTab.h"
#include "LogoPanel.h"

class AccordeonTabContainer : public Component
{
private:
    Array<AccordeonTab*> tabs;
    LogoPanel* logoPanel;

public:
    AccordeonTabContainer() : Component()
	{
        this->setBufferedToImage(true);
        this->logoPanel = NULL;

        this->setBufferedToImage(true);
	}

    ~AccordeonTabContainer() override
    {
        deleteAllChildren();
    }

    Array<AccordeonTab*> getAccordeonArray()
    {
        return tabs;
    }

    void resizeComponent()
    {
        setBounds(0, 0,  this->logoPanel->getWidth(), this->getMaxHeight());
    }

    void repaint()
    {
        for (int i = 0; i < tabs.size(); i++)
        {
            tabs[i]->repaint();
        }
    }

    int getMaxHeight()
    {
        // Fixme dynamic maximal number of open tabs
        int ySize = 0;
        for (int i = 0; i < 2; i++)
        {
            ySize += tabs[i]->getMaxHeight();
        }

        for (int i = 2; i < tabs.size(); i++)
        {
            ySize += tabs[i]->getTabHeight();
        }

        ySize += this->logoPanel->getHeight();
        return ySize;
    }

    void add(AccordeonTab *accordeonTab)
    {
        this->tabs.add(accordeonTab);
        addAndMakeVisible(accordeonTab);
    }

    void add(LogoPanel *logoPanel)
    {
        this->logoPanel = logoPanel;
        addAndMakeVisible(logoPanel);
    }

    int getCurrentHeight()
    {
       int ySize = 0;
        for (int i = 0; i < tabs.size(); i++)
        {
            ySize += this->tabs[i]->getTotalHeight();
        }

        return ySize + this->logoPanel->getHeight();
    }

    void resizeTabs(bool refresh)
    {
        int ySize = 0;
        for (int i = 0; i < tabs.size(); i++)
        {
            this->tabs[i]->setTabY(ySize, refresh);
            ySize += this->tabs[i]->getTotalHeight();
        }

        this->logoPanel->setPanelY(this->getMaxHeight() - this->logoPanel->getHeight());
    }
};
#endif
