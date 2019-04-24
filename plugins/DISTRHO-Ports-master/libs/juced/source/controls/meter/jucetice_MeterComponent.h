/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2009 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2007 by Julian Storer.

 ------------------------------------------------------------------------------

 JUCE and JUCETICE can be redistributed and/or modified under the terms of
 the GNU General Public License, as published by the Free Software Foundation;
 either version 2 of the License, or (at your option) any later version.

 JUCE and JUCETICE are distributed in the hope that they will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with JUCE and JUCETICE; if not, visit www.gnu.org/licenses or write to
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA

 ==============================================================================

   @author  Mike W. Smith
   @tweaker Lucio Asnaghi

 ==============================================================================
*/

#ifndef __JUCETICE_METERCOMPONENT_HEADER__
#define __JUCETICE_METERCOMPONENT_HEADER__

//==============================================================================
/**
    Creates VU meters - horizontal bar, vertical bar, and analog meter

*/
class MeterComponent : public Component,
                       public Timer
{
public:

    //==============================================================================
    /** The type of meters to use. */
    enum MeterType
    {
        MeterVertical,
        MeterHorizontal,
        MeterAnalog
    };

    //==============================================================================
    /** Needle drop shadow type */
    enum MeterNeedlePosition
    {
        NeedleNone,
        NeedleLeft,
        NeedleRight,
        NeedleAbove,
        NeedleBelow
    };

    //==============================================================================
    /** Creates a meter component.

        You can create vertical or horizontal VU meters that are smooth or segmented,
        or a simple analog style VU meter. The meters have built in sample and hold and
        decay, plus allow skewing for mapping linear input to logarithmic output.

        @param type                     Required - Vertical, Horizontal, or Analog style
        @param segments                 Number of segments to divide a bar graph meter into, or the thickness of the
                                        analog meter arc in pixels. Set to 0 for a smooth type meter. The width or height of a
                                        bar meter should be a multiple of this.
        @param markerWidth              number of pixels in between bars on segmented bar graph meters, or the width of the
                                        needle on analog meter.
        @param minColour                The color of a clean signal.
        @param thresholdColour          The color of a peak signal.
        @param maxColour                The color of a distorting signal.
        @param back                     The background color of the meter.
        @param threshold                The percentage point (between 0.0f and 1.0f) where the peak signal (threshold or 0dB) occurs
    */
    MeterComponent(
        int type,
        int segments=0,
        int markerWidth=2,
        const Colour& minColour=Colours::green,
        const Colour& thresholdColour=Colours::yellow,
        const Colour& maxColour=Colours::red,
        const Colour& back=Colours::black,
        float threshold=0.707f);

    //==============================================================================
    /** Creates an Analog meter component.

        This creates an analog style meter with background and overlay images.

        @param background               Required - Background image that needle will move over. Set to null for no image.
                                        Deleted in destructor.
        @param overlay                  Required - Overlay image that appears over the needle. Set to null for no image.
                                        Deleted in destructor.
        @param minPosition              Required - Position of needle when at rest, left = 0, straight up = 0.5, right = 1.0
        @param maxPostion               Required - Position of needle at peak position, left = 0, straight up = 0.5, right = 1.0
        @param needleCenter             Required - Needle X-Y pivot point
        @param needleLength             Required - Length of needle, in pixels
        @param needleWidth              Width of needle, in pixels
        @param arrowLength              Length of arrow at tip of needle, in pixels. Set to 0 for no arrow.
        @param arrowWidth               Width of arrow at tip of needle, in pixels. Set to 0 for no arrow.
        @param needleColour             Color of needle
        @param needleDropShadow         Use a drop shadow behind needle: None, Left, Right, Above, Below. Simulates the effect
                                        of a lamp inside a meter.
        @param dropDistance             Distance, in pixels, of a needle drop shadow
    */
    MeterComponent(
        Image background,
        Image overlay,
        float minPosition,
        float maxPosition,
        Point<int>& needleCenter,
        int needleLength,
        int needleWidth=2,
        int arrowLength=4,
        int arrowWidth=4,
        const Colour& needleColour=Colours::black,
        int needleDropShadow=NeedleNone,
        int dropDistance=0);

    /** Destructor */
    ~MeterComponent();

    /** Changes the component's position and size.

        The co-ordinates are relative to the top-left of the component's parent, or relative
        to the origin of the screen is the component is on the desktop.

        If this method changes the component's top-left position, it will make a synchronous
        call to moved(). If it changes the size, it will also make a call to resized().

        The width of a horizontal meter, or the height of a vertical meter, should always be a
        multiple of the number of segments if you want the bars to line up with the boundary.
    */
    void setBounds (int x, int y, int width, int height);

    /** Creates a beveled frame around the meter.

        @param inset                    Depth of the bevel, in pixels.
        @param raised                   Popped up or sunken.
    */
    void setFrame (int inset, bool raised = false);

    /** Change the colors of the meter

        @param minColour                The color of a clean signal.
        @param thresholdColour          The color of a peak signal.
        @param maxColour                The color of a distorting signal.
    */
    void setColours (Colour& min, Colour& threshold, Colour& max, Colour& back);

    /** Adjustable decay and hold time

        The defaults serve well for most situations. You can adjust the rate of decay,
        the peak hold time, and the percentage to decay.

        @param decay                    Decay time, in milliseconds (default is 50ms). Set to 0 to turn off.
        @param hold                     Number of decay periods to hold the peak (default is 4 periods, or 200ms)
        @param percent                  How much to decay each period (default is 0.707)
    */
    void setDecay (int decay, int hold, float percent=0.707f);

    /** Sets the meter value

        @param v                        A value between 0.0 and 1.0
    */
    void setValue (float v);

    /** Returns the current meter value
    */
    float getValue() const          { return m_value; }

    /** Sets the skew amount

        @param skew                     Change the ratio between the meter value and the position of the graph/needle (default is 1.0)
    */
    void setSkewFactor (float skew) { m_skew = skew; }

    //==============================================================================
    /** @internal */
    void paint (Graphics& g);
    /** @internal */
    void timerCallback();

    //==============================================================================
    juce_UseDebuggingNewOperator

protected:

    Image m_img;
    float m_value;
    float m_skew;
    float m_threshold;
    int m_meterType;
    int m_segments;
    int m_markerWidth;
    int m_inset;
    bool m_raised;
    Colour m_minColour;
    Colour m_thresholdColour;
    Colour m_maxColour;
    Colour m_backgroundColour;
    int m_decayTime;
    float m_decayPercent;
    float m_decayToValue;
    int m_hold;
    int m_monostable;
    Image m_background;
    Image m_overlay;
    float m_minPosition;
    float m_maxPosition;
    Point<int> m_needleCenter;
    int m_needleLength;
    int m_needleWidth;
    int m_arrowLength;
    int m_arrowWidth;
    Colour m_needleColour;
    int m_needleDropShadow;
    int m_dropDistance;

    //==============================================================================
    void buildImage(void);
};


#endif // __JUCETICE_METERCOMPONENT_HEADER__
