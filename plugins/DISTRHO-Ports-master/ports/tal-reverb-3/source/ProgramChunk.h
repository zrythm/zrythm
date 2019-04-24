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
		xmlChunk = T("<?xml version=\"1.0\" encoding=\"UTF-8\"?><tal curprogram=\"9\" version=\"1\">  <programs>    <program programname=\"Stereo Plate\" dry=\"0.5\" wet=\"0.230000004\" roomsize=\"0.41200003\"             predelay=\"0\" lowshelfgain=\"0.89200002\" highshelfgain=\"0.328000009\"             stereowidth=\"1\" realstereomode=\"1\" power=\"1\"/>    <program programname=\"Short Stereo Plate\" dry=\"0.5\" wet=\"0.32100001\" roomsize=\"0.236000016\"             predelay=\"0\" lowshelfgain=\"1\" highshelfgain=\"0.316000015\" stereowidth=\"1\"             realstereomode=\"1\" power=\"1\"/>    <program programname=\"High Plate\" dry=\"0.504000008\" wet=\"0.245000005\"             roomsize=\"0.460000008\" predelay=\"0\" lowshelfgain=\"0.572000027\"             highshelfgain=\"1\" stereowidth=\"1\" realstereomode=\"1\" power=\"1\"/>    <program programname=\"Long Mono Plate\" dry=\"0.5\" wet=\"0.363000005\" roomsize=\"0.756000042\"             predelay=\"0\" lowshelfgain=\"0.968000054\" highshelfgain=\"0.388000011\"             stereowidth=\"0\" realstereomode=\"1\" power=\"1\"/>    <program programname=\"Big Fx Plate\" dry=\"0.5\" wet=\"0.310000002\" roomsize=\"1\"             predelay=\"0\" lowshelfgain=\"1\" highshelfgain=\"1\" stereowidth=\"1\"             realstereomode=\"1\" power=\"1\"/>    <program programname=\"Air Plate\" dry=\"0.5\" wet=\"0.157000005\" roomsize=\"0.668000042\"             predelay=\"0\" lowshelfgain=\"0\" highshelfgain=\"1\" stereowidth=\"1\"             realstereomode=\"0\" power=\"1\"/>    <program programname=\"Bass Plate\" dry=\"0.5\" wet=\"0.33100003\" roomsize=\"0.604000032\"             predelay=\"0\" lowshelfgain=\"1\" highshelfgain=\"0\" stereowidth=\"1\"             realstereomode=\"1\" power=\"1\"/>    <program programname=\"Mono Air Plate\" dry=\"0.5\" wet=\"0.32100001\" roomsize=\"0.524000049\"             predelay=\"0\" lowshelfgain=\"0.504000008\" highshelfgain=\"0.508000016\"             stereowidth=\"0\" realstereomode=\"1\" power=\"1\"/>    <program programname=\"Short Mono Plate\" dry=\"0.5\" wet=\"0.352000028\" roomsize=\"0.228000015\"             predelay=\"0.0720000044\" lowshelfgain=\"0.824000061\" highshelfgain=\"0.064000003\"             stereowidth=\"0\" realstereomode=\"1\" power=\"1\"/>    <program programname=\"Gentle Plate\" dry=\"0.5\" wet=\"0.266000003\" roomsize=\"0.340000004\"             predelay=\"0\" lowshelfgain=\"0.756000042\" highshelfgain=\"0.248000011\"             stereowidth=\"1\" realstereomode=\"1\" power=\"1\"/>  </programs>  <midimap/></tal>");
	}

	const String getXmlChunk() 
	{
		return xmlChunk;
	}
	private:
	String xmlChunk;
};
#endif