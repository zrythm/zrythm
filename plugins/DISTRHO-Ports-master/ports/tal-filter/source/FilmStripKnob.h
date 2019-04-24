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

class FilmStripKnob : public Slider
{
public:
	FilmStripKnob(Image image, const int numFrames, const bool stripIsHorizontal, int index) 
        : Slider(juce::String(index)),
		filmStrip(image),
		numFrames_(numFrames),
		isHorizontal_(stripIsHorizontal)
	{
		setTextBoxStyle(NoTextBox, 0, 0, 0);
		setSliderStyle(RotaryVerticalDrag);
		frameHeight = filmStrip.getHeight() / numFrames_;
		frameWidth = filmStrip.getWidth();
		setRange(0.0f, 1.0f, 0.001f);

        getProperties().set(Identifier("index"), index);
	}

	void paint(Graphics& g) override
	{
			int value = (int)((getValue() - getMinimum()) / (getMaximum() - getMinimum()) * (numFrames_ - 1));
			if(isHorizontal_) 
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

private:
	Image filmStrip;
	const int numFrames_;
	const bool isHorizontal_;
	int frameWidth, frameHeight;
};