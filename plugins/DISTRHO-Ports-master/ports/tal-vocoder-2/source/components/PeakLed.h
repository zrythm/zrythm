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

class PeakLed : public Component, public Timer
{
public:
    PeakLed(const String& componentName, const Image filmStrip, TalCore* const ownerFilter)
        : Component(componentName), filmStrip(filmStrip), ownerFilter(ownerFilter)
    {
        this->frameHeight = filmStrip.getHeight() / 2;
        this->frameWidth = filmStrip.getWidth();

        this->ledEnabled = false;

        startTimer(20);
    }

    void paint(Graphics& g) override
    {
        int value = 0;

        if (this->ledEnabled)
        {
            value = 1;
        }

        g.drawImage(filmStrip, 0, 0, getWidth(), getHeight(), 0, value * frameHeight, frameWidth, frameHeight);
    }

    void timerCallback() override
    {
        bool clipps = ownerFilter->doesClip();

        if (clipps != ledEnabled)
        {
            repaint();
        }

        this->ledEnabled = clipps;
    }

private:
    const Image filmStrip;
    int frameWidth, frameHeight;

    TalCore* ownerFilter;

    bool ledEnabled;
};
