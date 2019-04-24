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
		xmlChunk = T("<?xml version=\"1.0\" encoding=\"UTF-8\"?><tal curprogram=\"2\" version=\"1\">  <programs>    <program programname=\"Gentle Drum Ambience\" dry=\"0.5\" wet=\"0.13000001\"             roomsize=\"0.660000026\" predelay=\"0.180000007\" lowshelffrequency=\"0.088000007\"             highshelffrequency=\"0.632000029\" peakfrequency=\"0.54400003\" lowshelfgain=\"0.936000049\"             highshelfgain=\"0.828000069\" peakgain=\"0.764000058\" stereowidth=\"1\"             realstereomode=\"1\"/>    <program programname=\"80's Plate\" dry=\"0.5\" wet=\"0.209000006\" roomsize=\"0.660000026\"             predelay=\"0.0600000024\" lowshelffrequency=\"0.5\" highshelffrequency=\"0.424000025\"             peakfrequency=\"0.368000031\" lowshelfgain=\"1\" highshelfgain=\"0.788000047\"             peakgain=\"0.368000031\" stereowidth=\"1\" realstereomode=\"0\"/>    <program programname=\"Big Wet Plate (no EQ)\" dry=\"0.5\" wet=\"0.161000013\"             roomsize=\"0.752000034\" predelay=\"0.131999999\" lowshelffrequency=\"0.0240000002\"             highshelffrequency=\"1\" peakfrequency=\"0.492000014\" lowshelfgain=\"1\"             highshelfgain=\"1\" peakgain=\"1\" stereowidth=\"1\" realstereomode=\"0\"/>    <program programname=\"Small Drum Plate\" dry=\"0.5\" wet=\"0.335000008\" roomsize=\"0.308000028\"             predelay=\"0.120000005\" lowshelffrequency=\"0.176000014\" highshelffrequency=\"0.444000036\"             peakfrequency=\"0.300000012\" lowshelfgain=\"0.724000037\" highshelfgain=\"0.488000035\"             peakgain=\"0\" stereowidth=\"1\" realstereomode=\"1\"/>    <program programname=\"Dull Plate\" dry=\"0.5\" wet=\"0.242000014\" roomsize=\"0.632000029\"             predelay=\"0.160000011\" lowshelffrequency=\"0.328000009\" highshelffrequency=\"0.720000029\"             peakfrequency=\"0.568000019\" lowshelfgain=\"1\" highshelfgain=\"0.216000006\"             peakgain=\"0.360000014\" stereowidth=\"1\" realstereomode=\"1\"/>    <program programname=\"Need some Air\" dry=\"0.5\" wet=\"0.149000004\" roomsize=\"0.688000023\"             predelay=\"0.120000005\" lowshelffrequency=\"0.212000012\" highshelffrequency=\"0.5\"             peakfrequency=\"0.536000013\" lowshelfgain=\"0.248000011\" highshelfgain=\"1\"             peakgain=\"0.644000053\" stereowidth=\"1\" realstereomode=\"0\"/>    <program programname=\"Mid Plate\" dry=\"0.5\" wet=\"0.371000022\" roomsize=\"0.604000032\"             predelay=\"0.208000004\" lowshelffrequency=\"0.324000001\" highshelffrequency=\"0.272000015\"             peakfrequency=\"0.576000035\" lowshelfgain=\"0.764000058\" highshelfgain=\"0\"             peakgain=\"0.112000003\" stereowidth=\"1\" realstereomode=\"1\"/>    <program programname=\"Airy Mono Plate\" dry=\"0.5\" wet=\"0.441000015\" roomsize=\"0.732000053\"             predelay=\"0.0720000044\" lowshelffrequency=\"0.180000007\" highshelffrequency=\"0.428000033\"             peakfrequency=\"0.320000023\" lowshelfgain=\"0.356000006\" highshelfgain=\"0.380000025\"             peakgain=\"0\" stereowidth=\"0\" realstereomode=\"0\"/>    <program programname=\"1100Hz Plate\" dry=\"0.5\" wet=\"0.404000014\" roomsize=\"0.600000024\"             predelay=\"0.212000012\" lowshelffrequency=\"0.352000028\" highshelffrequency=\"0.360000014\"             peakfrequency=\"1\" lowshelfgain=\"0\" highshelfgain=\"0\" peakgain=\"0.263999999\"             stereowidth=\"1\" realstereomode=\"0\"/>    <program programname=\"Short Plate\" dry=\"0.5\" wet=\"0.478000015\" roomsize=\"0.104000002\"             predelay=\"0.136000007\" lowshelffrequency=\"0.5\" highshelffrequency=\"0.5\"             peakfrequency=\"0.536000013\" lowshelfgain=\"0.492000014\" highshelfgain=\"0.356000006\"             peakgain=\"0.172000006\" stereowidth=\"1\" realstereomode=\"1\"/>  </programs>  <midimap/></tal>");
	}

	const String getXmlChunk() 
	{
		return xmlChunk;
	}
	private:
	String xmlChunk;
};
#endif