/*
  ==============================================================================

  This file is part of the dRowAudio JUCE module
  Copyright 2004-13 by dRowAudio.

  ------------------------------------------------------------------------------

  dRowAudio is provided under the terms of The MIT License (MIT):

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
  SOFTWARE.

  ==============================================================================
*/



SegmentedMeter::SegmentedMeter()
    : numRedSeg     (2),
      numYellowSeg  (4),
	  numGreenSeg   (9),
	  totalNumSegs  (numRedSeg + numYellowSeg + numGreenSeg),
	  decibelsPerSeg(3.0f),
	  numSegs       (0),
	  sampleCount   (0),
	  samplesToCount(2048),
	  sampleMax     (0.0f),
	  level         (0.0f),
	  needsRepaint  (true)
{
	setOpaque (true);
}

SegmentedMeter::~SegmentedMeter()
{
}

void SegmentedMeter::calculateSegments()
{
	float numDecibels = (float) toDecibels (level.getCurrent());
	// map decibels to numSegs
	numSegs = jmax (0, roundToInt ((numDecibels / decibelsPerSeg) + (totalNumSegs - numRedSeg)));
	
	// impliment slow decay
	//	level.set((0.5f * level.getCurrent()) + (0.1f * level.getPrevious()));
	level *= 0.8f;
	
	// only actually need to repaint if the numSegs has changed
	if (! numSegs.areEqual() || needsRepaint)
		repaint();
}

void SegmentedMeter::timerCallback()
{
	calculateSegments();
}

void SegmentedMeter::resized()
{
	const int m = 2;
	const int w = getWidth();
	const int h = getHeight();

    onImage = Image (Image::RGB,
                     w, h,
                     false);
    offImage = Image (Image::RGB,
                      w, h,
                      false);
    
    Graphics gOn (onImage);
    Graphics gOff (offImage);
    
    const int numSegments = (numRedSeg + numYellowSeg + numGreenSeg);
	const float segmentHeight = (h - m) / (float) numSegments;
    const float segWidth = w - (2.0f * m);
	
	for (int i = 1; i <= numSegments; ++i)
	{
		if (i <= numGreenSeg)
		{
			gOn.setColour (Colours::green.brighter (0.8f));
            gOff.setColour (Colours::green.darker());
		}
		else if (i <= (numYellowSeg + numGreenSeg))
		{
			gOn.setColour (Colours::orange.brighter());
            gOff.setColour (Colours::orange.darker());
		}
		else
		{
			gOn.setColour (Colours::red.brighter());
            gOff.setColour (Colours::red.darker());
		}

		gOn.fillRect ((float) m, h - m - (i * segmentHeight), segWidth, segmentHeight);
		gOn.setColour (Colours::black);
		gOn.drawLine ((float) m, h - m - (i * segmentHeight), (float) w - m, h - m - (i * segmentHeight), (float) m);
		
        gOff.fillRect ((float) m, h - m - (i * segmentHeight), segWidth, segmentHeight);
		gOff.setColour (Colours::black);
		gOff.drawLine ((float) m, h - m - (i * segmentHeight), (float) w - m, h - m - (i * segmentHeight), (float) m);
	}
	
	gOn.setColour (Colours::black);
	gOn.drawRect (0, 0, w, h, m);    

    gOff.setColour (Colours::black);
	gOff.drawRect (0, 0, w, h, m);    

    needsRepaint = true;
}

void SegmentedMeter::paint (Graphics &g)
{
	const int w = getWidth();
	const int h = getHeight();

    if (onImage.isValid()) 
    {
        const int onHeight = roundToInt ((numSegs.getCurrent() / (float) totalNumSegs) * onImage.getHeight());
        const int offHeight = h - onHeight;
        
//        g.drawImage (onImage,
//                     0, offHeight, w, onHeight, 
//                     0, offHeight, w, onHeight,
//                     false);
//
//        g.drawImage (offImage,
//                     0, 0, w, offHeight, 
//                     0, 0, w, offHeight,
//                     false);
        g.drawImage (onImage,
                     0, 0, w, h,
                     0, 0, w, h,
                     false);
        
        g.drawImage (offImage,
                     0, 0, w, offHeight, 
                     0, 0, w, offHeight,
                     false);
    }
    
    needsRepaint = false;
}

void SegmentedMeter::process()
{
	// calculate current meter level
	if (samples.getData() != nullptr)
	{
		for (int i = 0; i < numSamples; ++i)
		{
			float sample = fabsf (samples[i]);
			
			if (sample > sampleMax)
				sampleMax = sample;
			
			if (++sampleCount == samplesToCount) 
            {
				if (sampleMax > level.getCurrent())
					level = sampleMax;
				
				sampleMax = 0.0f;
				sampleCount = 0;
			}
		}
	}
}

