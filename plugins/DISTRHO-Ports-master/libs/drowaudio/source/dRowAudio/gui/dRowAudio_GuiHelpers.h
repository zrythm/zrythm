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

#ifndef __DROWAUDIO_GUIHELPERS_H__
#define __DROWAUDIO_GUIHELPERS_H__

#if JUCE_MSVC
    #pragma warning (disable: 4505)
#endif

namespace GuiHelpers
{
    /** Creates a base colour for a component based on the current keyboard
        and mouse interactivity.
     */
    inline static Colour createBaseColour (const Colour& colour,
                                           const bool hasKeyboardFocus,
                                           const bool isMouseOver,
                                           const bool isButtonDown) noexcept
    {
        const float sat = hasKeyboardFocus ? 1.3f : 0.9f;
        const Colour baseColour (colour.withMultipliedSaturation (sat));
        
        if (isButtonDown)
            return baseColour.contrasting (0.2f);
        else if (isMouseOver)
            return baseColour.contrasting (0.1f);
        
        return baseColour;
    }
    
    /** Draws a square bevel around a given rectange.
        This is useful for insetting components and givin them a border.
     */
    inline static void drawBevel (Graphics& g, Rectangle<float> innerBevelBounds,
                                  float bevelThickness, const Colour& baseColour)
    {
       Rectangle<float> outerBevelBounds (innerBevelBounds.expanded (bevelThickness, bevelThickness));
       Rectangle<float> centreBevelBounds (innerBevelBounds.expanded (bevelThickness * 0.5f, bevelThickness * 0.5f));
        
        Path pL, pR, pT, pB, pTL, pTR, pBL, pBR;
        pL.startNewSubPath (centreBevelBounds.getTopLeft());
        pL.lineTo (centreBevelBounds.getBottomLeft());
        
        pR.startNewSubPath (centreBevelBounds.getTopRight());
        pR.lineTo (centreBevelBounds.getBottomRight());
        
        pT.startNewSubPath (centreBevelBounds.getTopLeft());
        pT.lineTo (centreBevelBounds.getTopRight());
        
        pB.startNewSubPath (centreBevelBounds.getBottomLeft());
        pB.lineTo (centreBevelBounds.getBottomRight());
        
        pTL.addTriangle (outerBevelBounds.getX(), outerBevelBounds.getY(),
                         outerBevelBounds.getX(), innerBevelBounds.getY(),
                         innerBevelBounds.getX(), innerBevelBounds.getY());
        
        pTR.addTriangle (outerBevelBounds.getRight(), outerBevelBounds.getY(),
                         outerBevelBounds.getRight(), innerBevelBounds.getY(),
                         innerBevelBounds.getRight(), innerBevelBounds.getY());
        
        pBL.addTriangle (outerBevelBounds.getX(), outerBevelBounds.getBottom(),
                         outerBevelBounds.getX(), innerBevelBounds.getBottom(),
                         innerBevelBounds.getX(), innerBevelBounds.getBottom());
        
        pBR.addTriangle (outerBevelBounds.getRight(), innerBevelBounds.getBottom(),
                         outerBevelBounds.getRight(), outerBevelBounds.getBottom(),
                         innerBevelBounds.getRight(), innerBevelBounds.getBottom());
        
        g.saveState();
        
        g.setColour (baseColour);
        g.strokePath (pL, PathStrokeType (bevelThickness));
        g.strokePath (pR, PathStrokeType (bevelThickness));
        
        g.setColour (baseColour.darker (0.5f));
        g.strokePath (pT, PathStrokeType (bevelThickness, PathStrokeType::mitered, PathStrokeType::square));
        
        g.setColour (baseColour.brighter (0.5f));
        g.strokePath (pB, PathStrokeType (bevelThickness, PathStrokeType::mitered, PathStrokeType::square));
        
        g.setColour (baseColour);
        g.fillPath (pTL);
        g.fillPath (pTR);
        g.fillPath (pBL);
        g.fillPath (pBR);
        
        g.restoreState();
    }  
    
    //==============================================================================
    /**
        Helper function to serialise a system font to a file.
        This is useful if you want to include a custom font in an application so that is
        guarenteed to be avialiable cross-platform.
     
        Once you have the generated file you can include it as BinarayData (as generated
        by theIntorjucer) and then reload it in a look and feel.
     
        In your LookAndFeel subclass overide getTypeFaceForFont and create a Typeface::Ptr
        to hold the font e.g.
     
        @code
        //==============================================================================
        class FontLookAndFeel : public LookAndFeel
        {
        public:
            const Typeface::Ptr getTypefaceForFont (const Font& font);
     
        private:
            Typeface::Ptr customTypeface;
        }
        @endcode
     
        Then in your constructor reload the font and the return your custom typeface
        in getTypefaceForFont if the requested font names match.
     
        @code
        //==============================================================================
        FontLookAndFeel::FontLookAndFeel()
        {
            ScopedPointer<MemoryInputStream> fontStream (new MemoryInputStream (BinaryData::Custom_Font,
                                                                                BinaryData::Custom_FontSize,
                                                                                false));
            
            if (fontStream != nullptr)
                customTypeface = new CustomTypeface (*fontStream);
        }

        //==============================================================================
        const Typeface::Ptr FontLookAndFeel::getTypefaceForFont (const Font& font)
        {
            if (customTypeface != nullptr && font.getTypefaceName() == customTypeface->getName())
            {
                return customTypeface;
            }
            
            return LookAndFeel::getTypefaceForFont (font);
        }
        @endcode
     
        @param font             The font to serialise.
        @param destinationFile  The file to serialise the font to.
        @param maxNumChars      The maximum number of characters to serialise.
        @return                 True if the font was written successfully, false otherwise
     
        @see Font, CustomTypeface
     */
    inline static bool serializeFont (const Font& font, File& destinationFile, int maxNumChars = 127)
    {
        destinationFile.deleteFile();
        ScopedPointer<FileOutputStream> outFileStream (destinationFile.createOutputStream());
        
        CustomTypeface customTypeface;
        customTypeface.setCharacteristics (font.getTypefaceName(), font.getAscent(),
                                           font.isBold(), font.isItalic(), ' ');
        customTypeface.addGlyphsFromOtherTypeface (*font.getTypeface(), 0, maxNumChars);

        return customTypeface.writeToStream (*outFileStream);
    }

    //==============================================================================
    /**
        List of icons to be used with the createIcon method.
     */
    enum IconType 
    {
		Stop,
		Play,
		Cue,
		Pause,
		Next,
		Previous,
		ShuffleForward,
		ShuffleBack,
		Eject,
		Cross,
		Add,
		Search,
		Power,
		Bypass,
		GoUp,
		Infinity,
		DownTriangle,
		Info,
        Loop,
        Slow,
        Speaker,
        MutedSpeaker,
		noIcons
	};
    
    /** Creates an icon in a given colour.
     */
    inline static DrawablePath createIcon (IconType icon, Colour colour)
    {
        switch (icon) 
        {
            case Stop:
            {
                Path squarePath;
                squarePath.addRectangle (100.0f, 100.0f, 100.0f, 100.0f);
                
                DrawablePath squareImage;
                squareImage.setFill (colour);
                squareImage.setPath (squarePath);
                
                return squareImage;
            }
                break;
            case Play:
            {
                Path trianglePath;
                trianglePath.addTriangle (0.0f, 0.0f, 0.0f, 100.0f, 100.0f, 50.0f);
                
                DrawablePath triangleImage;
                triangleImage.setFill (colour);
                triangleImage.setPath (trianglePath);
                
                return triangleImage;
            }
            case Pause:
            {
                Path pausePath;
                pausePath.addRectangle (0.0f, 0.0f, 20.0f, 100.0f);
                pausePath.addRectangle (60.0f, 0.0f, 20.0f, 100.0f);
                
                DrawablePath pauseImage;
                pauseImage.setFill (colour);
                pauseImage.setPath (pausePath);
                
                return pauseImage;
            }
                break;
            case Cue:
            {
                Path p;
                p.addRectangle (0, 0, 30, 50);
                p.addArrow (Line<float> (0.0f, 50.0f, 100.0f, 50.0f), 30.0f, 100.0f, 40.0f);
                
                DrawablePath drawablePath;
                drawablePath.setFill (colour);
                drawablePath.setPath (p);
                
                return drawablePath;
            }
                break;
            case Next:
            {
                Path p;
                p.addTriangle (0.0f, 0.0f, 0.0f, 100.0f, 90.0f, 50.0f);
                p.addRectangle (90, 0, 10, 100);
                
                DrawablePath drawablePath;
                drawablePath.setFill (colour);
                drawablePath.setPath (p);
                
                return drawablePath;
            }
            case Previous:
            {
                Path p;
                p.addTriangle (100.0f, 100.0f, 100.0f, 0.0f, 10.0f, 50.0f);
                p.addRectangle (0, 0, 10, 100);
                
                DrawablePath drawablePath;
                drawablePath.setFill (colour);
                drawablePath.setPath (p);
                
                return drawablePath;
            }
                break;
            case ShuffleForward:
            {
                Path p;
                p.addTriangle (0.0f, 0.0f, 0.0f, 100.0f, 50.0f, 50.0f);
                p.addTriangle (50.0f, 0.0f, 50.0f, 100.0f, 100.0f, 50.0f);
                
                DrawablePath drawablePath;
                drawablePath.setFill (colour);
                drawablePath.setPath (p);
                
                return drawablePath;
            }			
                break;
            case ShuffleBack:
            {
                Path p;
//                p.addTriangle (50.0f, 0.0f, 50.0f, 100.0f, 0.0f, 50.0f);
//                p.addTriangle (100.0f, 0.0f, 100.0f, 100.0f, 50.0f, 50.0f);
//                p.addTriangle (25.0f, 0.0f, 25.0f, 50.0f, 0.0f, 25.0f);
//                p.addTriangle (100.0f, 0.0f, 100.0f, 50.0f, 50.0f, 25.0f);
                p.addTriangle (0.0f, 50.0f, 75.0f, 0.0f, 75.0f, 100.0f);
                p.addTriangle (75.0f, 50.0f, 150.0f, 0.0f, 150.0f, 100.0f);
                
                DrawablePath drawablePath;
                drawablePath.setFill (colour);
                drawablePath.setPath (p);
                
                return drawablePath;
            }
                break;
            case Eject:
            {
                Path p;
                p.addTriangle (0, 65, 100, 65, 50, 0);
                p.addRectangle (0, 80, 100, 20);
                
                DrawablePath drawablePath;
                drawablePath.setFill (colour);
                drawablePath.setPath (p);
                
                return drawablePath;			
            }
                break;
            case Cross:
            {
                Path p;
                p.startNewSubPath (0.0f, 0.0f);
                p.lineTo (100.0f, 100.0f);
                p.startNewSubPath (100.0f, 0.0f);
                p.lineTo (0.0f, 100.0f);
                
                DrawablePath drawablePath;
                drawablePath.setFill (Colours::white.withAlpha (0.0f));
                drawablePath.setStrokeFill (colour);
                drawablePath.setStrokeThickness (15);
                drawablePath.setPath (p);
                
                return drawablePath;			
            }
                break;
            case Add:
            {
                Path p;
                p.startNewSubPath (50.0f, 0.0f);
                p.lineTo (50.0f, 100.0f);
                p.startNewSubPath (0.0f, 50.0f);
                p.lineTo (100.0f, 50.0f);
                
                DrawablePath drawablePath;
                drawablePath.setFill (Colours::white.withAlpha (0.0f));
                drawablePath.setStrokeFill (colour);
                drawablePath.setStrokeThickness (15);
                drawablePath.setPath (p);
                
                return drawablePath;			
            }
                break;
            case Search:
            {
                Path p;
                p.addEllipse (20, 0, 80, 80);
                p.startNewSubPath (0.0f, 100.0f);
                p.lineTo (35.0f, 65.0f);
                
                DrawablePath drawablePath;
                drawablePath.setFill (Colours::white.withAlpha (0.0f));
                drawablePath.setStrokeFill (colour);
                drawablePath.setStrokeThickness (15);
                drawablePath.setPath (p);
                
                return drawablePath;			
            }
                break;
            case Power:
            {
                Path p;
                p.addArc (0.0f, 20.0f, 100.0f, 100.0f, 0.18f * float_Pi, 2.0f * float_Pi - (0.18f * float_Pi), true);
                p.startNewSubPath (50.0f, 0.0f);
                p.lineTo (50.0f, 70.0f);
                
                DrawablePath drawablePath;
                drawablePath.setFill (Colours::white.withAlpha (0.0f));
                drawablePath.setStrokeFill (colour);
                drawablePath.setStrokeThickness (10.0f);
                drawablePath.setPath (p);
                
                return drawablePath;
            }
                break;
            case Bypass:
            {
                Path p;
                p.startNewSubPath (50.0f, 0.0f);
                p.lineTo (50.0f, 30.0f);
                p.lineTo (80.0f, 70.0f);
                p.startNewSubPath (50.0f, 70.0f);
                p.lineTo (50.0f, 100.0f);
                
                DrawablePath drawablePath;
                drawablePath.setFill (Colours::white.withAlpha (0.0f));
                drawablePath.setStrokeFill (colour);
                drawablePath.setStrokeThickness (10);
                drawablePath.setPath (p);
                
                return drawablePath;			
            }
                break;
            case GoUp:
            {
                Path arrowPath;
                arrowPath.addArrow (Line<float> (50.0f, 100.0f, 50.0f, 0.0f), 40.0f, 100.0f, 50.0f);
                
                DrawablePath arrowImage;
                arrowImage.setFill (colour);
                arrowImage.setPath (arrowPath);
                
                return arrowImage;			
            }
                break;
            case Infinity:
            {
                Path infPath;
                infPath.addEllipse (0.0f, 0.0f, 50.0f, 50.0f);
                infPath.startNewSubPath (50.0f, 0.0f);
                infPath.addEllipse (50.0f, 0.0f, 50.0f, 50.0f);
                
                DrawablePath infImg;
                infImg.setFill (Colours::white.withAlpha (0.0f));
                infImg.setStrokeFill (colour);
                infImg.setStrokeThickness (10.0f);
                infImg.setPath (infPath);
                
                return infImg;			
            }
                break;
            case DownTriangle:
            {
                Path trianglePath;
                trianglePath.addTriangle (0.0f, 0.0f, 100.0f, 0.0f, 50.0f, 100.0f);
                
                DrawablePath triangleImage;
                triangleImage.setFill (colour);
                triangleImage.setPath (trianglePath);
                
                return triangleImage;
            }
                break;
            case Info:
            {
                Path circlePath;
                circlePath.addEllipse (0.0f, 0.0f, 100.0f, 100.0f);
                
                DrawablePath circleImage;
                circleImage.setFill (colour);
                circleImage.setPath (circlePath);
                
                return circleImage;
            }
                break;
            case Loop:
            {
                Path loopPath;
                loopPath.addRoundedRectangle (0.0f, 0.0f, 150.0f, 50.0f, 25.0f);
                loopPath.addTriangle (100.0f, -10.0f, 100.0f, 10.0f, 110.0f, 0.0f);
                loopPath.addTriangle (40.0f, 50.0f, 50.0f, 60.0f, 50.0f, 40.0f);
                
                DrawablePath loopImage;
                loopImage.setFill (Colours::white.withAlpha(0.0f));
                loopImage.setStrokeFill (colour);
                loopImage.setStrokeThickness (15.0f);
                loopImage.setPath (loopPath);
                
                return loopImage;
            }
                break;
            case Slow:
            {
                Path p;
                p.addPolygon (Point<float> (50.0f, 50.0f),
                              8,
                              50.0f,
                              -float_Pi * 0.125f);
                
                //            GlyphArrangement text;
                //            text.addLineOfText (Font (100), "SLOW", 0, 0);
                //            
                //            Path p2;
                //            text.createPath (p2);
                //            p.addPath (p2);
                
                DrawablePath dp;
                dp.setFill (colour);
                dp.setStrokeFill (colour);
                dp.setStrokeThickness (0.0f);
                dp.setPath (p);
                
                return dp;
            }
                break;
            case Speaker:
            {
                Path p;
                p.addRoundedRectangle (0.0f, 33.0f, 33.0f, 33.0f, 2.0f);
                p.addTriangle (7.5f, 50.0f, 55.0f, 6.5f, 55.0f, 93.5f);
                p.addArc (60.0f, 30.0f, 12.0f, 40.0f, float_Pi * 0.15f, float_Pi * 0.85f, true);
                p.addArc (70.0f, 20.0f, 16.0f, 60.0f, float_Pi * 0.15f, float_Pi * 0.85f, true);
                p.addArc (80.0f, 10.0f, 20.0f, 80.0f, float_Pi * 0.15f, float_Pi * 0.85f, true);
                
                DrawablePath dp;
                dp.setFill (colour);
                dp.setStrokeFill (colour);
                dp.setStrokeThickness (5.0f);
                dp.setPath (p);
                
                return dp;
            }
                break;
            case MutedSpeaker:
            {
                Path p;
                p.addRoundedRectangle (0.0f, 33.0f, 33.0f, 33.0f, 2.0f);
                p.addTriangle (7.5f, 50.0f, 55.0f, 6.5f, 55.0f, 93.5f);
                p.scaleToFit (0.0f, 0.0f, 100.0f, 100.0f, true);
                //            p.addEllipse (0.0f, 0.0f, 100.0f, 100.0f);
                //            p.addLineSegment (Line<float> (15.0f, 15.0f, 85.0f, 85.0f), 8.0f);
                
                DrawablePath dp;
                dp.setFill (colour);
                dp.setStrokeFill (colour);
                dp.setStrokeThickness (5.0f);
                dp.setPath (p);
                
                return dp;
            }
                break;
            default:
            {
                DrawablePath blank;
                return blank;
            }
                break;
        }
    }
}

#endif // __DROWAUDIO_GUIHELPERS_H__