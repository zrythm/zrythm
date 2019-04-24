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

BEGIN_JUCE_NAMESPACE

//==============================================================================
MeterComponent::MeterComponent(
                               int type,
                               int segs,
                               int markerWidth,
                               const Colour& min,
                               const Colour& thresh,
                               const Colour& max,
                               const Colour& back,
                               float threshold) :
    Component("Meter Component"),
    m_img(0),
    m_value(0),
    m_skew(1.0f),
    m_threshold(threshold),
    m_meterType(type),
    m_segments(segs),
    m_markerWidth(markerWidth),
    m_inset(0),
    m_raised(false),
    m_minColour(min),
    m_thresholdColour(thresh),
    m_maxColour(max),
    m_backgroundColour(back),
    m_decayTime(50),
    m_decayPercent(0.707f),
    m_decayToValue(0),
    m_hold(4),
    m_monostable(0),
    m_background(0),
    m_overlay(0),
    m_minPosition(0.1f),
    m_maxPosition(0.9f),
    m_needleLength(0),
    m_needleWidth(markerWidth)
{
    if(m_meterType == MeterAnalog)
        if(!m_segments)
            // Minimum analog meter wiper width
            m_segments = 8;
    startTimer(m_decayTime);
}

//==============================================================================
MeterComponent::MeterComponent(Image background,
                               Image overlay,
                               float minPosition,
                               float maxPosition,
                               Point<int>& needleCenter,
                               int needleLength,
                               int needleWidth,
                               int arrowLength,
                               int arrowWidth,
                               const Colour& needleColour,
                               int needleDropShadow,
                               int dropDistance) :
    Component("Meter Component"),
    m_img(0),
    m_value(0),
    m_skew(1.0f),
    m_threshold(0.707f),
    m_meterType(MeterAnalog),
    m_segments(0),
    m_markerWidth(0),
    m_inset(0),
    m_raised(false),
    m_minColour(0),
    m_thresholdColour(0),
    m_maxColour(0),
    m_backgroundColour(0),
    m_decayTime(50),
    m_decayPercent(0.707f),
    m_decayToValue(0),
    m_hold(4),
    m_monostable(0),
    m_background(0),
    m_overlay(0),
    m_minPosition(minPosition),
    m_maxPosition(maxPosition),
    m_needleLength(needleLength),
    m_needleWidth(needleWidth),
    m_arrowLength(arrowLength),
    m_arrowWidth(arrowWidth),
    m_needleDropShadow(needleDropShadow),
    m_dropDistance(dropDistance)
{
    m_background = background;
    m_overlay = overlay;
    m_needleCenter.setXY(needleCenter.getX(), needleCenter.getY());
    m_needleColour = needleColour;

    startTimer(m_decayTime);
}

//==============================================================================
MeterComponent::~MeterComponent()
{
    stopTimer();
}

//==============================================================================
void MeterComponent::buildImage(void)
{
    // Build our image in memory
    int w = getWidth() - m_inset*2;
    int h = getHeight() - m_inset*2;

    if(m_meterType == MeterHorizontal)
    {
        m_img = Image(Image(Image::RGB, w, h, false));
        Graphics g(m_img);

        g.setGradientFill (ColourGradient(m_minColour, 0, 0, m_thresholdColour, w * m_threshold, 0, false));
        g.fillRect(0, 0, int(w * m_threshold), h);

        g.setGradientFill (ColourGradient(m_thresholdColour, int(w * m_threshold), 0, m_maxColour, w, 0, false));
        g.fillRect(int(w * m_threshold), 0, w, h);

        if(m_segments)
        {
            // Break up our image into segments
            g.setColour (m_backgroundColour);
            g.fillRect (0, 0, w, m_markerWidth);
            g.fillRect (0, h-m_markerWidth, w, m_markerWidth);
            for(int i = 0; i <= m_segments; i++)
                g.fillRect(i * (w/m_segments), 0, m_markerWidth, h);
        }
    }
    else if(m_meterType == MeterVertical)
    {
        m_img = Image(Image::RGB, w, h, false);
        Graphics g(m_img);

        int hSize = (int)(h * m_threshold);
        g.setGradientFill (ColourGradient(m_minColour, 0, h, m_thresholdColour, 0, h - hSize, false));
        g.fillRect(0, h - hSize, w, hSize);

        g.setGradientFill (ColourGradient(m_thresholdColour, 0, h - hSize, m_maxColour, 0, 0, false));
        g.fillRect(0, 0, w, h - hSize);

        if(m_segments)
        {
            // Break up our image into segments
            g.setColour (m_backgroundColour);
            g.fillRect (0, 0, m_markerWidth, h);
            g.fillRect (w-m_markerWidth, 0, m_markerWidth, h);
            for(int i = 0; i <= m_segments; i++)
                g.fillRect(0, i * (h/m_segments), w, m_markerWidth);
        }
    }
    // Only build if no other analog images exist
    else if(m_meterType == MeterAnalog && !(m_background.isValid() || m_overlay.isValid() || m_needleLength))
    {
        m_img = Image(Image::RGB, w, h, false);
        Graphics g(m_img);

        g.setColour(m_backgroundColour);
        g.fillRect(0, 0, w, h);

        const double left = 4.71238898;     // 270 degrees in radians
        const double right = 1.57079633;    // 90 degrees in radians
        double startPos = m_minPosition;    // Start at 10%
        double endPos = m_maxPosition;      // End at 90%

        float strokeWidth = m_segments;     // We get our wiper width from the segments amount
        double pos;
        float radius = jmax (w/2, h/2) - strokeWidth/2;
        float angle;
        float x;
        float y;

        // Create an arc with lineTo's (works better than addArc)
        Path p;
        for(pos = startPos; pos < endPos; pos += .02)
        {
            angle = left + pos * (right - left);
            x = sin(angle)*radius + w/2;
            y = cos(angle)*radius + h - m_segments;
            if(pos == startPos)
                p.startNewSubPath(x, y);
            else
                p.lineTo(x, y);
        }
        angle = left + pos * (right - left);
        p.lineTo(sin(angle)*radius + w/2, cos(angle)*radius + h - m_segments);

        // Create an image brush of our gradient
        Image img(Image::RGB, w, h, false);
        Graphics g2(img);

        g2.setGradientFill (ColourGradient(m_minColour, 0, 0, m_thresholdColour, w * m_threshold, 0, false));
        g2.fillRect(0, 0, int(w * m_threshold), h);

        g2.setGradientFill (ColourGradient(m_thresholdColour, int(w * m_threshold), 0, m_maxColour, w, 0, false));
        g2.fillRect(int(w * m_threshold), 0, w, h);

        // Stroke the arc with the gradient
        g.setTiledImageFill (img, 0, 0, 1.0);
        g.strokePath(p, PathStrokeType(strokeWidth));
    }
}

//==============================================================================
void MeterComponent::setBounds (int x, int y, int width, int height)
{
    Component::setBounds(x, y, width, height);
    buildImage();
}

//==============================================================================
void MeterComponent::setFrame(int inset, bool raised)
{
    m_inset=inset;
    m_raised=raised;
    buildImage();
}

//==============================================================================
void MeterComponent::setColours(Colour& min, Colour& threshold, Colour& max, Colour& back)
{
    m_minColour = min;
    m_thresholdColour = threshold;
    m_maxColour = max;
    m_backgroundColour = back;
    buildImage();
}

//==============================================================================
void MeterComponent::setValue(float v)
{
    float val = jmin(jmax(v, 0.0f), 1.0f);
    if (m_skew != 1.0 && val > 0.0)
        val = exp (log (val) / m_skew);

    if(m_decayTime)
    {
        if(m_value < val)
        {
            // Sample and hold
            m_monostable=m_hold;
            m_value = val;
            repaint();
        }
        m_decayToValue = val;
    }
    else
    {
        if(m_value != val)
        {
            m_value = val;
            repaint();
        }
    }
}

//==============================================================================
void MeterComponent::setDecay(int decay, int hold, float percent)
{
    m_decayPercent = percent;
    m_decayTime = decay;
    m_hold = hold;
    if(m_decayTime)
        // Start our decay
        startTimer(m_decayTime);
    else
        stopTimer();
}

//==============================================================================
void MeterComponent::timerCallback()
{
    if(m_monostable)
        // Wait for our hold period
        m_monostable--;
    else
    {
        m_value = m_decayToValue + (m_decayPercent * (m_value - m_decayToValue));
        if(m_value - m_decayToValue > 0.01f)
            // Only repaint if there's enough changes
            repaint();
        else if(m_value != m_decayToValue)
        {
            // Zero out
            m_value = m_decayToValue;
            repaint();
        }
    }
}

//==============================================================================
//
//    This needs to be made more efficient by only repainting when things actually
//    change, and only painting the area that has changed. Right now, this paints
//    everything on every repaint request.
//
void MeterComponent::paint (Graphics& g)
{
    int h = getHeight() - m_inset*2;
    int w = getWidth() - m_inset*2;

    // Background
    if(!m_background.isValid() && !m_needleLength)
    {
        g.setColour(m_backgroundColour);
        g.fillRect(m_inset, m_inset, w, h);
    }

    // Bevel outline for the entire draw area
    if(m_inset)
        LookAndFeel_V2::drawBevel(
            g,
            0,
            0,
            getWidth(),
            getHeight(),
            m_inset,
            m_raised?Colours::white.withAlpha(0.4f):Colours::black.withAlpha(0.4f),
            m_raised?Colours::black.withAlpha(0.4f):Colours::white.withAlpha(0.4f));

    // Blit our prebuilt image
    g.setOpacity(1.0f);
    if(m_meterType == MeterHorizontal)
    {
        if(m_segments)
        {
            float val = float(int(m_value * m_segments)) / m_segments;
            g.drawImage(m_img, m_inset, m_inset, (int) (w * val), h, 0, 0, (int) (w * val), h);
        }
        else
            g.drawImage(m_img, m_inset, m_inset, (int) (w * m_value), h, 0, 0, (int) (w * m_value), h);
    }
    else if(m_meterType == MeterVertical)
    {
        int hSize;
        if(m_segments)
            hSize = (int) (h * float(int(m_value * m_segments)) / m_segments);
        else
            hSize = (int) (h * m_value);
        g.drawImage(m_img, m_inset, m_inset + h - hSize, w, hSize, 0, h - hSize, m_img.getWidth(), hSize);
    }
    else if(m_meterType == MeterAnalog)
    {
        if(m_background.isValid())
            g.drawImage(m_background, m_inset, m_inset, w, h, 0, 0, m_background.getWidth(), m_background.getHeight());
        else if(m_img.isValid())
            g.drawImage(m_img, m_inset, m_inset, w, h, 0, 0, m_img.getWidth(), m_img.getHeight());

        float angle = 4.71238898 + (m_value * (m_maxPosition - m_minPosition) + m_minPosition) * -3.14159265;
        if(m_needleLength)
        {
            g.setColour(m_needleColour);
			Line<float> l (m_needleCenter.getX() + m_inset,
						   m_needleCenter.getY() + m_inset,
						   sin(angle)*m_needleLength + m_needleCenter.getX() + m_inset,
						   cos(angle)*m_needleLength + m_needleCenter.getY() + m_inset);
            g.drawArrow (l, m_needleWidth, m_arrowLength, m_arrowWidth);

            if(m_needleDropShadow)
            {
                int dropX = 0, dropY = 0, dropPointX = 0, dropPointY = 0;
                
                if(m_needleDropShadow == NeedleLeft)
                {
                    dropX = -m_dropDistance;
                    dropY = 0;
                    dropPointX = -m_dropDistance;
                    dropPointY = 0;
                }
                else if(m_needleDropShadow == NeedleRight)
                {
                    dropX = m_dropDistance;
                    dropY = 0;
                    dropPointX = m_dropDistance;
                    dropPointY = 0;
                }
                else if(m_needleDropShadow == NeedleAbove)
                {
                    dropX = 0;
                    dropY = 0;
                    dropPointX = 0;
                    dropPointY = -m_dropDistance;
                }
                else if(m_needleDropShadow == NeedleBelow)
                {
                    dropX = 0;
                    dropY = m_dropDistance;
                    dropPointX = 0;
                    dropPointY = m_dropDistance;
                }
                g.setColour(Colours::black.withAlpha(0.2f));
				Line<float> l (m_needleCenter.getX() + m_inset + dropX,
							   m_needleCenter.getY() + m_inset + dropY,
							   sin(angle)*m_needleLength + m_needleCenter.getX() + m_inset + dropPointX,
							   cos(angle)*m_needleLength + m_needleCenter.getY() + m_inset + dropPointY);
                g.drawArrow(l, m_needleWidth, m_arrowLength, m_arrowWidth);
            }
        }
        else
        {
            // Contrasting arrow for built-in analog meter
            g.setColour(m_backgroundColour.contrasting(1.0f).withAlpha(0.9f));
			Line<float> l (w/2 + m_inset,
						   h - m_segments + m_inset,
						   sin(angle)*jmax (w/2, h/2) + w/2 + m_inset,
						   cos(angle)*jmax (w/2, h/2) + h - m_segments + m_inset);
            g.drawArrow(l, m_needleWidth, m_needleWidth*2, m_needleWidth*2);
        }

        if(m_overlay.isValid())
        {
            g.setOpacity(1.0f);
            g.drawImage(m_overlay, m_inset, m_inset, w, h, 0, 0, m_overlay.getWidth(), m_overlay.getHeight());
        }
    }
}

END_JUCE_NAMESPACE
