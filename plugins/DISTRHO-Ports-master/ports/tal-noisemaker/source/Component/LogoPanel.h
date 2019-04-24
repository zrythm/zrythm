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

#ifndef LogoPanel_H
#define LogoPanel_H

class LogoPanel : public Component
{
private:
    Image image;
    int startY;
    int height;

public:
    LogoPanel(Image image, int startY, int height) : Component()
	{
       this->image = image;
       this->startY = startY;
       this->height = height;

       setPanelY(0);
	}

    ~LogoPanel() override
    {
        deleteAllChildren();
    }

    void setPanelY(int y)
    {
        setBounds(0, y, image.getWidth(), height);
    }

    void paint(Graphics& g) override
    {
        g.drawImage (image,
                     0, 0, image.getWidth(), height,
                     0, startY, image.getWidth(), height);
    }
};
#endif
