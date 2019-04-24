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

#ifndef __DROWAUDIO_COLUMNFILEBROWSERLOOKANDFEEL_H__
#define __DROWAUDIO_COLUMNFILEBROWSERLOOKANDFEEL_H__

//==================================================================================
/** The LookAndFeel of the ColumnFileBrowser.
    This is not intended for public use.
 */
class ColumnFileBrowserLookAndFeel : public LookAndFeel_V2
{
public:
    //==================================================================================
	ColumnFileBrowserLookAndFeel()
	{
		setColour (ListBox::backgroundColourId, Colour::greyLevel (0.2f));
		setColour (DirectoryContentsDisplayComponent::highlightColourId, Colour::greyLevel (0.9f));
		setColour (DirectoryContentsDisplayComponent::textColourId, Colour::greyLevel (0.9f));		
		
		// scrollbars
		setColour (ScrollBar::backgroundColourId, Colour::greyLevel (0.5f));
		setColour (ScrollBar::thumbColourId, Colour::greyLevel (0.8f));
		setColour (ScrollBar::trackColourId, Colour::greyLevel (0.3f));		
	}
	
	~ColumnFileBrowserLookAndFeel()
	{
	}
	
	void layoutFileBrowserComponent (FileBrowserComponent& browserComp,
									 DirectoryContentsDisplayComponent* fileListComponent,
									 FilePreviewComponent* /*previewComp*/,
									 ComboBox* /*currentPathBox*/,
									 TextEditor* /*filenameBox*/,
									 Button* /*goUpButton*/)
	{
		int w = browserComp.getWidth();
		int x = 2;
		int y = 2;
		
		Component* const listAsComp = dynamic_cast <Component*> (fileListComponent);
		listAsComp->setBounds (x, y, w, browserComp.getHeight() - y);// - bottomSectionHeight);
		
		y = listAsComp->getBottom();
	}
	
	void drawFileBrowserRow (Graphics& g, int width, int height,
							 const String& filename, Image* icon,
							 const String& fileSizeDescription,
							 const String& fileTimeDescription,
							 bool isDirectory,
							 bool isItemSelected,
							 int /*itemIndex*/,
							 DirectoryContentsDisplayComponent& /*component*/)
	{
		if (isItemSelected)
			g.fillAll (findColour (DirectoryContentsDisplayComponent::highlightColourId));
		//		else if ((itemIndex % 2) != 0)
		//			g.fillAll (findColour(ListBox::backgroundColourId).withBrightness(0.95f));
		//		else if ((itemIndex % 2) == 0)
		//			g.fillAll (browserColumnRow.withAlpha(0.05f));
		
        //		g.setColour (findColour (DirectoryContentsDisplayComponent::textColourId));
        //		if (isItemSelected)
        //			g.setColour (findColour (DirectoryContentsDisplayComponent::textColourId).contrasting(1.0f));
        //		g.setFont (height * 0.7f);
        //		
        //		Image im;
        //		if (icon != 0 && icon->isValid())
        //			im = *icon;
        //		
        //		if (im.isNull())
        //			im = isDirectory ? getDefaultFolderImage()
        //			: getDefaultDocumentFileImage();
        //		
        //		const int x = 32;
        //		
        //		if (im.isValid())
        //		{
        //			g.drawImageWithin (im, 2, 2, x - 4, height - 4,
        //							   RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize,
        //							   false);
        //		}
		const int x = 32;
		g.setColour (Colours::black);
		
		if (icon != 0 && icon->isValid())
		{
			g.drawImageWithin (*icon, 2, 2, x - 4, height - 4,
							   RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize,
							   false);
		}
		else
		{
			const Drawable* d = isDirectory ? getDefaultFolderImage()
			: getDefaultDocumentFileImage();
			
			if (d != 0)
				d->drawWithin (g, Rectangle<float> (2.0f, 2.0f, x - 4.0f, height - 4.0f),
							   RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize, 1.0f);
		}
		
		g.setColour (findColour (DirectoryContentsDisplayComponent::textColourId));
		if (isItemSelected)
			g.setColour (findColour (DirectoryContentsDisplayComponent::highlightColourId).contrasting(1.0f));
        
		g.setFont (height * 0.7f);
		
		if (width > 450 && ! isDirectory)
		{
			const int sizeX = roundFloatToInt (width * 0.7f);
			const int dateX = roundFloatToInt (width * 0.8f);
			
			g.drawFittedText (filename,
							  x, 0, sizeX - x, height,
							  Justification::centredLeft, 1);
			
			g.setFont (height * 0.5f);
			g.setColour (Colours::darkgrey);
			
			if (! isDirectory)
			{
				g.drawFittedText (fileSizeDescription,
								  sizeX, 0, dateX - sizeX - 8, height,
								  Justification::centredRight, 1);
				
				g.drawFittedText (fileTimeDescription,
								  dateX, 0, width - 8 - dateX, height,
								  Justification::centredRight, 1);
			}
		}
		else
		{
			g.drawFittedText (filename,
							  x, 0, (int) (width - x - (height * 0.7f)), height,
							  Justification::centredLeft, 1);
			
		}
		
		// draw directory triangles
		if (isDirectory)
		{			
			int diameter = roundToInt (height * 0.5f);
			Path p;
			p.addTriangle (width - (height * 0.2f), height * 0.5f,
						   (float) (width - diameter), height * 0.3f,
						   (float) (width - diameter), height * 0.7f);
			g.setColour (findColour (ScrollBar::thumbColourId));
			g.fillPath (p);
			
			g.setColour (Colour (0x80000000));
			g.strokePath (p, PathStrokeType (0.5f));
		}
	}
	
	//============================================================
	void drawScrollbarButton (Graphics& g,
                              ScrollBar& scrollbar,
                              int width, int height,
                              int buttonDirection,
                              bool /*isScrollbarVertical*/,
                              bool /*isMouseOverButton*/,
                              bool isButtonDown)
	{
		Path p;
		
		if (buttonDirection == 0)
			p.addTriangle (width * 0.5f, height * 0.2f,
						   width * 0.1f, height * 0.7f,
						   width * 0.9f, height * 0.7f);
		else if (buttonDirection == 1)
			p.addTriangle (width * 0.8f, height * 0.5f,
						   width * 0.3f, height * 0.1f,
						   width * 0.3f, height * 0.9f);
		else if (buttonDirection == 2)
			p.addTriangle (width * 0.5f, height * 0.8f,
						   width * 0.1f, height * 0.3f,
						   width * 0.9f, height * 0.3f);
		else if (buttonDirection == 3)
			p.addTriangle (width * 0.2f, height * 0.5f,
						   width * 0.7f, height * 0.1f,
						   width * 0.7f, height * 0.9f);
		
		if (isButtonDown)
			g.setColour (scrollbar.findColour (ScrollBar::thumbColourId).contrasting (0.2f));
		else
			g.setColour (scrollbar.findColour (ScrollBar::thumbColourId));
		
		g.fillPath (p);
		
		g.setColour (Colour (0x80000000));
		g.strokePath (p, PathStrokeType (0.5f));
	}
	
	void drawScrollbar (Graphics& g,
                        ScrollBar& scrollbar,
                        int x, int y,
                        int width, int height,
                        bool isScrollbarVertical,
                        int thumbStartPosition,
                        int thumbSize,
                        bool /*isMouseOver*/,
                        bool /*isMouseDown*/)
	{
		g.fillAll (scrollbar.findColour (ScrollBar::backgroundColourId));
		
		Path slotPath, thumbPath;
		
		float gx1 = 0.0f, gy1 = 0.0f, gx2 = 0.0f, gy2 = 0.0f;
		
		if (isScrollbarVertical)
		{
			slotPath.addRoundedRectangle ((float) x,
										  (float) y,
										  (float) width,
										  (float) height,
										  width * 0.5f);
			
			if (thumbSize > 0)
				thumbPath.addRoundedRectangle ((float) x,
											   (float) thumbStartPosition,
											   (float) width,
											   (float) thumbSize,
											   width * 0.5f);
			gx1 = (float) x;
			gx2 = x + width * 0.7f;
		}
		else
		{
			slotPath.addRoundedRectangle ((float) x,
										  (float) y,
										  (float) width,
										  (float) height,
										  height * 0.5f);
			
			if (thumbSize > 0)
				thumbPath.addRoundedRectangle ((float) thumbStartPosition,
											   (float) y,
											   (float) thumbSize,
											   (float) height,
											   height * 0.5f);
			gy1 = (float) y;
			gy2 = y + height * 0.7f;
		}
		
		const Colour thumbColour (scrollbar.findColour (ScrollBar::thumbColourId));
		Colour trackColour1, trackColour2;
		
		trackColour1 = scrollbar.findColour (ScrollBar::trackColourId);
		trackColour2 = trackColour1.overlaidWith (Colour (0x19000000));
		
		g.setGradientFill (ColourGradient (trackColour1, gx1, gy1,
										   trackColour2, gx2, gy2, false));
		g.fillPath (slotPath);
		
		if (isScrollbarVertical)
		{
			gx1 = x + width * 0.6f;
			gx2 = (float) x + width;
		}
		else
		{
			gy1 = y + height * 0.6f;
			gy2 = (float) y + height;
		}
		
		g.setGradientFill (ColourGradient (Colours::transparentBlack,gx1, gy1,
										   Colour (0x19000000), gx2, gy2, false));
		g.fillPath (slotPath);
		
		g.setColour (thumbColour);
		g.fillPath (thumbPath);
		
		//    g.setGradientFill (ColourGradient (Colour (0x10000000), gx1, gy1,
		//									   Colours::transparentBlack, gx2, gy2, false));
		//	
		//    g.saveState();
		//	
		//    if (isScrollbarVertical)
		//        g.reduceClipRegion (x + width / 2, y, width, height);
		//    else
		//        g.reduceClipRegion (x, y + height / 2, width, height);
		//	
		//    g.fillPath (thumbPath);
		//    g.restoreState();
		
		//    g.setColour (Colour (0x4c000000));
		//    g.setColour (thumbColour.brighter(0.5));
		//    g.strokePath (thumbPath, PathStrokeType (1.0f));
	}
	
	//============================================================
    void drawCornerResizer (Graphics& g,
                            int w, int h,
                            bool /*isMouseOver*/,
                            bool /*isMouseDragging*/)
	{
		const float lineThickness = 1.0f;//jmin (w, h) * 0.075f;
		const float xGap = w / 3.0f;
		const float yGap = h * 0.25f;
		
		g.setColour (findColour (ScrollBar::backgroundColourId));
		g.fillAll();
		
		g.setColour (findColour (ScrollBar::thumbColourId));
		g.drawLine (xGap + lineThickness * 0.5f, yGap, xGap + lineThickness * 0.5f, h - yGap, lineThickness);
		g.drawLine ((2 * xGap) - lineThickness * 0.5f, yGap, (2 * xGap) - lineThickness * 0.5f, h - yGap, lineThickness);
	}
};

#endif //__DROWAUDIO_COLUMNFILEBROWSERLOOKANDFEEL_H__