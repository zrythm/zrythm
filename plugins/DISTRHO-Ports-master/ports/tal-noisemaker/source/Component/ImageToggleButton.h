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

class ImageToggleButton : public Button, public Timer
{
public:
    // On Off button takes a filmstrip with two images
    ImageToggleButton(const String& componentName, const Image filmStrip, const bool stripIsHorizontal, const bool isKickButton, int index)
        : Button(componentName), filmStrip(filmStrip), isHorizontal(stripIsHorizontal), isKickButton(isKickButton)
    {
        frameHeight = filmStrip.getHeight() / 2;
        frameWidth = filmStrip.getWidth();
        setClickingTogglesState (true);

        getProperties().set(Identifier("index"), index);
    }

    ~ImageToggleButton() override
    {
	    clearShortcuts();
    }

    void paintButton(Graphics& g, bool isMouseOverButton, bool isButtonDown) override
    {
        int value = 0;
        if (this->getToggleState())
        {
            value = 1;
        }
        if(isHorizontal)
        {
            g.drawImage(filmStrip, 0, 0, getWidth(), getHeight(),
                value * frameWidth, 0, frameWidth, frameHeight);
        }
        else
        {
            g.drawImage(filmStrip, 0, 0, getWidth(), getHeight(),
                0, value * frameHeight, frameWidth, frameHeight);
        }
    }

    void timerCallback() override
    {
        stopTimer();
        setToggleState(false, dontSendNotification);
        repaint();
    }

    void clicked() override
    {
        if (isKickButton)
        {
            this->setToggleState(true, dontSendNotification);
            Button::clicked();
            if (true)
            {
                startTimer(200);
            }
        }
    }

private:
    const Image filmStrip;
    const bool isHorizontal;
    const bool isKickButton;

    int frameWidth, frameHeight;
};
