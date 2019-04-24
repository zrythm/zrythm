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

#ifndef __DROWAUDIO_XMLHELPERS_H__
#define __DROWAUDIO_XMLHELPERS_H__

namespace XmlHelpers
{
    //==============================================================================
    /** Adds an XML style comment to a string.
        Useful if you're constructing a string to be parsed into and XmlElement.
     */
    inline static String& addXmlComment (String& string, const String& comment)
    {
        return string << "<!-- " << comment << " -->" << newLine;
    }

    //==============================================================================
    /** Searches an XmlElement for an element with a given attribute name with
        the given attribute value.
     */
    inline static XmlElement* findXmlElementWithAttributeWithValue (XmlElement* element,
                                                             const String& attributeName,
                                                             const String& attributeValue)
    {
        if (element == nullptr)
            return nullptr;
        
        if (element->hasAttribute (attributeName))
            if (element->compareAttribute (attributeName, attributeValue, true))
                return element;
        
        XmlElement* child = element->getFirstChildElement();
        
        while (child != nullptr)
        {
            if (child->hasAttribute (attributeName))
                if(element->compareAttribute (attributeName, attributeValue, true))
                    return element;
            
            XmlElement* const found = findXmlElementWithAttributeWithValue (child, attributeName, attributeValue);
            
            if (found != nullptr)
                return found;
            
            child = child->getNextElement();
        }
        
        return nullptr;
    }

    //==============================================================================
    /** Searches an XmlElement for an element with a given attribute name.
     */
    inline static XmlElement* findXmlElementWithAttribute (XmlElement* element, const String& attributeName)
    {
        if (element == nullptr)
            return nullptr;
        
        if (element->hasAttribute (attributeName))
            return element;
        
        XmlElement* child = element->getFirstChildElement();
        
        while (child != nullptr)
        {
            if (child->hasAttribute (attributeName))
                return element;
            
            XmlElement* const found = findXmlElementWithAttribute (child, attributeName);
            
            if (found != nullptr)
                return found;
            
            child = child->getNextElement();
        }
        
        return nullptr;
    }

    //==============================================================================
    /** Searches for an element with subtext that is an exact match.
     */
    inline static XmlElement* findXmlElementWithSubText (XmlElement* element, const String& subtext)
    {
        if (element == nullptr)
            return nullptr;
        
        if (element->getAllSubText() == subtext)
            return element;
        
        XmlElement* child = element->getFirstChildElement();
        
        while (child != nullptr)
        {
            if (child->getAllSubText() == subtext)
                return child;
            
            XmlElement* const found = findXmlElementWithSubText (child, subtext);
            
            if (found != nullptr)
                return found;
            
            child = child->getNextElement();
        }
        
        return nullptr;
    }
    
    //==============================================================================
    /** Searches for an element with subtext contains the given text.
     */
    inline static XmlElement* findXmlElementContainingSubText (XmlElement* element, const String& subtext)
    {
        if (element == nullptr || element->getFirstChildElement() == nullptr)
            return nullptr;
        
        if (element->getFirstChildElement()->isTextElement()
            && element->getFirstChildElement()->getText().contains (subtext))
            return element;
        
        XmlElement* child = element->getFirstChildElement();
        
        while (child != nullptr)
        {
            if (child->isTextElement()
                && child->getText().contains (subtext))
                return child;
            
            XmlElement* const found = findXmlElementContainingSubText (child, subtext);
            
            if (found != nullptr)
                return found;
            
            child = child->getNextElement();
        }
        
        return nullptr;
    }
}

#endif  // __DROWAUDIO_XMLHELPERS_H__
