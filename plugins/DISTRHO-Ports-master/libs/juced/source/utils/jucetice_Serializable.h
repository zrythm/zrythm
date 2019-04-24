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
*/

#ifndef __JUCETICE_SERIALIZABLE_HEADER__
#define __JUCETICE_SERIALIZABLE_HEADER__

//==============================================================================
template<class TypeName>
class Serializable
{
public:

	/** Virtual destructor */
	virtual ~Serializable () {}

	/** Subclass this for serialize an object in a XML element
	*/
	virtual TypeName* serialize () = 0;

	/** Subclass this for restore an object from a XML element
	*/
	virtual void restore (TypeName* e) = 0;
};


//==============================================================================
typedef Serializable<XmlElement> XmlSerializable;

class SerializeFactoryXml
{
public:

	/** constructor */
	SerializeFactoryXml () {}

	/** Virtual destructor */
	~SerializeFactoryXml () { objects.clear(); }

	/** Register a serializable with this factory */
	void addSerializable (XmlSerializable* object);

	/** Removes a serializable object from the factory */
	void removeSerializable (XmlSerializable* object);

	/** Serialize all the objects registered */
	XmlElement* serializeObjects (const String& rootName);

	/** Restored the objects registered with the root element */
	void restoreObjects (XmlElement* root);

	juce_DeclareSingleton (SerializeFactoryXml, true)

protected:

	Array<XmlSerializable*> objects;
};


#endif // __JUCETICE_SERIALIZABLE_HEADER__
