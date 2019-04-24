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


#ifndef ProgramChunk_H
#define ProgramChunk_H

class ProgramChunk
{

public:
	ProgramChunk()  
	{
		xmlChunk = T("<?xml version=\"1.0\" encoding=\"UTF-8\"?><tal curprogram=\"0\" version=\"1\">  <programs>    <program programname=\"Smooth Plate\" dry=\"0.853000045\" wet=\"0.387000024\"             roomsize=\"0.556000054\" predelay=\"0\" damp=\"0\" lowcut=\"0.216000006\"             highcut=\"1\" stereowidth=\"1\"/>    <program programname=\"Gentle Drum Plate\" dry=\"0.847000062\" wet=\"0.380000025\"             roomsize=\"0.452000022\" predelay=\"0\" damp=\"0.0240000002\" lowcut=\"0.208000004\"             highcut=\"1\" stereowidth=\"1\"/>    <program programname=\"Big Drum Plate\" dry=\"0.847000062\" wet=\"0.411000013\"             roomsize=\"0.644000053\" predelay=\"0\" damp=\"0\" lowcut=\"0.228000015\"             highcut=\"0.792000055\" stereowidth=\"1\"/>    <program programname=\"Small Drum Plate\" dry=\"0.840000033\" wet=\"0.42900002\"             roomsize=\"0.208000004\" predelay=\"0\" damp=\"0\" lowcut=\"0\" highcut=\"1\"             stereowidth=\"1\"/>    <program programname=\"Big Kick FX Plate\" dry=\"0.883000016\" wet=\"0.632000029\"             roomsize=\"1\" predelay=\"0\" damp=\"0\" lowcut=\"0\" highcut=\"1\" stereowidth=\"1\"/>    <program programname=\"Gentle Mono Plate\" dry=\"0.840000033\" wet=\"0.454000026\"             roomsize=\"0.536000013\" predelay=\"0\" damp=\"0\" lowcut=\"0.131999999\"             highcut=\"1\" stereowidth=\"0\"/>    <program programname=\"Damped Plate\" dry=\"0.779000044\" wet=\"0.436000019\"             roomsize=\"0.552000046\" predelay=\"0\" damp=\"0.552000046\" lowcut=\"0.164000005\"             highcut=\"1\" stereowidth=\"1\"/>    <program programname=\"Ambience\" dry=\"0.816000044\" wet=\"0.270000011\" roomsize=\"0.600000024\"             predelay=\"0\" damp=\"0\" lowcut=\"0\" highcut=\"1\" stereowidth=\"1\"/>    <program programname=\"Thin Ambience\" dry=\"0.834000051\" wet=\"0.325000018\"             roomsize=\"0.60800004\" predelay=\"0\" damp=\"0.244000018\" lowcut=\"0.168000013\"             highcut=\"1\" stereowidth=\"1\"/>    <program programname=\"80er Plate\" dry=\"0.834000051\" wet=\"0.405000031\"             roomsize=\"0.640000045\" predelay=\"0\" damp=\"0.192000002\" lowcut=\"0\"             highcut=\"1\" stereowidth=\"1\"/>  </programs></tal>");
	}

	const String getXmlChunk() 
	{
		return xmlChunk;
	}
	private:
	String xmlChunk;
};
#endif