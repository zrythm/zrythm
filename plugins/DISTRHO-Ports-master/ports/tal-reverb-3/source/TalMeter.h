/*
	==============================================================================
	This file is part of Tal-Reverb by Patrick Kunz.

	Copyright(c) 2005-2009 Patrick Kunz, TAL
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

#include "./Engine/AudioUtils.h"

class TalMeter : public Component, public Timer
{
public:
	// Curently only supports vertical sliders
	TalMeter(TalCore* const ownerFilter, int totalWidth, int totalHeight, int numberOfLeds)
		: Component("TalMeter")
	{
		ledWidth = totalHeight / (numberOfLeds + 1);
		valueL = valueR = 0.0f;
		pad = 8;

		this->totalWidth = totalWidth;
		this->totalHeight = totalHeight;
		this->numberOfLeds = numberOfLeds;

		ledColourOn = ledColourOn.fromRGBA(
			(const juce::uint8)0, 
			(const juce::uint8)200,
			(const juce::uint8)0,
			(const juce::uint8)255);

		ledColourOnRed = ledColourOnRed.fromRGBA(
			(const juce::uint8)200, 
			(const juce::uint8)0,
			(const juce::uint8)0,
			(const juce::uint8)255);

		ledColourOnYellow = ledColourOnRed.fromRGBA(
			(const juce::uint8)200, 
			(const juce::uint8)200,
			(const juce::uint8)0,
			(const juce::uint8)255);

		ledColourOff = ledColourOff.fromRGBA(
			(const juce::uint8)30, 
			(const juce::uint8)30,
			(const juce::uint8)30,
			(const juce::uint8)255);

        startTimer(30);
		this->ownerFilter = ownerFilter;
	}

    void timerCallback()
    {
        float* newLevels = ownerFilter->getCurrentVolume();

        newLevels[0] = audioUtils.getLogScaledValueInverted(newLevels[0]);
        newLevels[1] = audioUtils.getLogScaledValueInverted(newLevels[1]);

        if (newLevels[0] >= valueL) valueL = newLevels[0];
        if (newLevels[1] >= valueR) valueR = newLevels[1];

		repaint();

        valueL -= 0.004f;
        valueR -= 0.004f;

        if (valueL < 0.0f) valueL = 0.0f;
        if (valueR < 0.0f) valueR = 0.0f;
    }

	void paint(Graphics& g)
	{
        this->drawMeterLeds(g, valueL, pad);
        this->drawMeterLeds(g, valueR, getLedWidth() + 2 * pad);
	}

    void drawMeterLeds(Graphics& g, float value, int xPos)
    {
		int intMaxMeterValue = (int)(1.0f * totalHeight - pad * 4);
		int intCurrentMeterValue = (int)(value * intMaxMeterValue);
		int intRedStart = (int)(0.96f * intMaxMeterValue);
		int intYellowStart = (int)(0.7f * intMaxMeterValue);
		for (int i = 0; i < intMaxMeterValue; i += 4)
		{
            if (i < intCurrentMeterValue && i <= intYellowStart)
            {
			    this->drawGlowLed(g, xPos, totalHeight - i - 2 * pad, ledColourOn, true);
            }
            else if (i < intCurrentMeterValue && i > intRedStart)
            {
			    this->drawGlowLed(g, xPos, totalHeight - i - 2 * pad, ledColourOnRed, true);
            }
            else if (i < intCurrentMeterValue && i > intYellowStart)
            {
			    this->drawGlowLed(g, xPos, totalHeight - i - 2 * pad, ledColourOnYellow, true);
            }
            else
            {
			    this->drawGlowLed(g, xPos, totalHeight - i - 2 * pad, ledColourOff, false);
            }
		}
    }

	void drawGlowLed(Graphics& g, int x, int y, Colour colour, float isGlowOn)
	{
        g.setColour(colour.darker(1.0f));
		g.fillRect(x, y, getLedWidth(), 3);

        g.setColour(colour);
		g.fillRect(x + 2, y + 1, getLedWidth() - 4, 1);

        if (isGlowOn)
        {
            g.setColour(colour.brighter(0.3f));
		    g.fillRect(x + 3, y + 1, 3, 1);
        }
	}

    int getLedWidth()
    {
        return (int)(totalWidth / 2.0f - 1.5f * pad);
    }

private:
	int totalWidth; 
	int totalHeight;
	int numberOfLeds;

	int ledWidth;
	int pad;

	Colour ledColourOn;
    Colour ledColourOnYellow;
	Colour ledColourOnRed;
	Colour ledColourOff;

	float valueL, valueR;
	TalCore* ownerFilter;

    AudioUtils audioUtils;
};
