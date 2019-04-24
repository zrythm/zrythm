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

#if DROWAUDIO_UNIT_TESTS

#include <modules/juce_core/juce_core.h>

//==============================================================================
class CumulativeMovingAverageTests  : public UnitTest
{
public:
    CumulativeMovingAverageTests() : UnitTest ("CumulativeMovingAverage") {}
    
    void runTest()
    {
        beginTest ("CumulativeMovingAverage");
        
        CumulativeMovingAverage average;
        
        expectEquals (average.add (1.0), 1.0);
        expectEquals (average.add (2.0), 1.5);
        expectEquals (average.add (3.0), 2.0);
        expectEquals (average.add (4.0), 2.5);
        expectEquals (average.add (5.0), 3.0);
        
        expectEquals (average.getNumValues(), 5);
    }
};

static CumulativeMovingAverageTests cumulativeMovingAverageUnitTests;

//==============================================================================
class MathsUnitTests  : public UnitTest
{
public:
    MathsUnitTests() : UnitTest ("Maths Utilities") {}
    
    void runTest()
    {
        beginTest ("Maths Utilities");

        expectEquals ((int) isEven (0), (int) true);
        expectEquals ((int) isEven (4), (int) true);
        expectEquals ((int) isEven (746352), (int) true);
        expectEquals ((int) isEven (-0), (int) true);
        expectEquals ((int) isEven (-4), (int) true);
        expectEquals ((int) isEven (-746352), (int) true);

        expectEquals ((int) isOdd (1), (int) true);
        expectEquals ((int) isOdd (23), (int) true);
        expectEquals ((int) isOdd (1763523), (int) true);
        expectEquals ((int) isOdd (-1), (int) true);
        expectEquals ((int) isOdd (-23), (int) true);
        expectEquals ((int) isOdd (-1763523), (int) true);

		expectEquals ((int) isnan (1), (int) false);
        expectEquals ((int) isnan (sqrt (-1.0)), (int) true);
        
        // RMS
        {
            const int numSamples = 512;
            AudioSampleBuffer asb (1, numSamples);

            // Sin
            {
                const double delta = numSamples / (2 * double_Pi);
                float* data = asb.getWritePointer (0);

                for (int i = 0; i < numSamples; ++i)
                    *data++ = (float) std::sin (i * delta);

                const float rms = findRMS<float> (asb.getReadPointer (0), numSamples);
                expect (almostEqual (rms, 0.707f, 0.001f));
            }
            
            // Square
            {
                float* data = asb.getWritePointer (0);

                for (int i = 0; i < numSamples; ++i)
                    *data++ = isOdd (i) ? 1.0f : -1.0f;

                const float rms = findRMS<float> (asb.getReadPointer (0), numSamples);
                expect (almostEqual (rms, 1.0f, 0.001f));
            }
        }
        
        // Reciprocal
        {
            Reciprocal<float> r;
            expectEquals (r.get(), 1.0f);
            expectEquals (r.getReciprocal(), 1.0f);
            
            Reciprocal<double> r2 (10.0);
            expectEquals (r2.get(), 10.0);
            expectEquals (r2.getReciprocal(), 1.0 / 10.0);
            expectEquals (r2 * 10, 100.0);
            expectEquals (r2 *= 10, 100.0);
            expectEquals (r2.get(), 100.0);
            expectEquals (r2.getReciprocal(), 1.0 / 100.0);
            
            expectEquals (r2 / 10, 10.0);
            expectEquals (r2.get(), 100.0);
            expectEquals (r2.getReciprocal(), 1.0 / 100.0);
            
            expectEquals (r2 /= 10, 10.0);
            expectEquals (r2.get(), 10.0);
            expectEquals (r2.getReciprocal(), 1.0 / 10.0);
        }
    }
};

static MathsUnitTests mathsUnitTests;

//==============================================================================
class PitchTests  : public UnitTest
{
public:
    PitchTests() : UnitTest ("Pitch") {}
    
    void runTest()
    {
        beginTest ("Pitch");
        
        Pitch pitch (Pitch::fromFrequency (440));
        expectEquals (pitch.getFrequencyHz(), 440.0);
        expectEquals (pitch.getMidiNote(), 69.0);
        expectEquals (pitch.getMidiNoteName(), String ("A4"));

        pitch = Pitch::fromMidiNote (68);
        expect (almostEqual (pitch.getFrequencyHz(), 415.304698, 0.000001));
        expectEquals (pitch.getMidiNote(), 68.0);
        expectEquals (pitch.getMidiNoteName(), String ("G#4"));
        
        pitch = Pitch::fromNoteName ("A#3");
        expect (almostEqual (pitch.getFrequencyHz(), 116.54094, 0.000001));
        expectEquals (pitch.getMidiNote(), 46.0);
        expectEquals (pitch.getMidiNoteName(), String ("A#2"));
        
        String fFlatThree;
        fFlatThree << "F" << Pitch::getFlatSymbol() << "2";
        pitch = Pitch::fromNoteName (fFlatThree);
        expect (almostEqual (pitch.getFrequencyHz(), 41.2034446, 0.000001));
        expectEquals (pitch.getMidiNote(), 28.0);
        expectEquals (pitch.getMidiNoteName(), String ("E1"));
    }
};

static PitchTests pitchTests;

//==============================================================================

#endif // DROWAUDIO_UNIT_TESTS
