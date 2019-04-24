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

class TalSplashScreen : public Button
{


public:

	// Curently only supports vertical sliders
	TalSplashScreen(Image image, int x, int y, int width, int height) : Button(T("asdf"))
	{
        this->image = image;

        this->x = x;
        this->y = y;
        this->width = width;
        this->height = height;

        this->setCurrentBounds();
	}

	void paintButton(Graphics& g, bool isMouseOverButton, bool isButtonDown) override
	{
        if (this->getToggleState())
        {
            g.drawImage(image, 0, 0, image.getWidth(), image.getHeight(), 0, 0, image.getWidth(), image.getHeight());
        }
	}

	void mouseDown(const MouseEvent &e) override
    {
        this->setToggleState(!this->getToggleState(), sendNotification);
        this->setCurrentBounds();
        repaint();
    }

    void setCurrentBounds()
    {
        if (this->getToggleState())
        {
            this->setBounds(0 ,0, image.getWidth(), image.getHeight());
        }
        else
        {
            this->setBounds(x ,y, width, height);
        }
    }

private:
    Image image;

    int x;
    int y;
    int width;
    int height;
};
