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
		xmlChunk = T("<?xml version=\"1.0\" encoding=\"UTF-8\"?><tal curprogram=\"0\" version=\"1\">  <programs>    <program programname=\"Clean Set Up\" cutoff=\"1\" resonance=\"0.432000011\"             filtertype=\"1\" lfointensity=\"0.512000024\" lforate=\"0.460000008\"             lfosync=\"4\" lfowaveform=\"1\" volume=\"0.860000014\" inputdrive=\"0.516000032\"             envelopeintensity=\"0.509000003\" envelopespeed=\"0\" lfowidth=\"0\"/>    <program programname=\"Lfo Sine 1/4 LP\" cutoff=\"0.772000015\" resonance=\"0.484000027\"             filtertype=\"1\" lfointensity=\"0.263999999\" lforate=\"0.280000001\"             lfosync=\"4\" lfowaveform=\"1\" volume=\"0.844000041\" inputdrive=\"0.496000022\"             envelopeintensity=\"0.508000016\" envelopespeed=\"0.656000018\" lfowidth=\"0\"/>    <program programname=\"Lfo 2/1 BP\" cutoff=\"0.572000027\" resonance=\"0.624000013\"             filtertype=\"3\" lfointensity=\"0.744000018\" lforate=\"0.244000018\"             lfosync=\"7\" lfowaveform=\"2\" volume=\"0.860000014\" inputdrive=\"0.527999997\"             envelopeintensity=\"0.496000022\" envelopespeed=\"0\" lfowidth=\"0.216000006\"/>    <program programname=\"Fast Env Guitar\" cutoff=\"0.360000014\" resonance=\"0.444000036\"             filtertype=\"1\" lfointensity=\"0.332000017\" lforate=\"0.432000011\"             lfosync=\"1\" lfowaveform=\"2\" volume=\"0.780000031\" inputdrive=\"0.54400003\"             envelopeintensity=\"0.836000025\" envelopespeed=\"0.148000002\" lfowidth=\"0.212000012\"/>    <program programname=\"Random Lfo  HP\" cutoff=\"0.448000014\" resonance=\"0.724000037\"             filtertype=\"2\" lfointensity=\"0.744000018\" lforate=\"0.548000038\"             lfosync=\"15\" lfowaveform=\"5\" volume=\"0.784000039\" inputdrive=\"0.340000004\"             envelopeintensity=\"0.508000016\" envelopespeed=\"0.508000016\" lfowidth=\"0\"/>    <program programname=\"Inverted  BP Env\" cutoff=\"0.780000031\" resonance=\"0.492000014\"             filtertype=\"3\" lfointensity=\"0.508000016\" lforate=\"0.476000011\"             lfosync=\"4\" lfowaveform=\"2\" volume=\"0.824000061\" inputdrive=\"0.508000016\"             envelopeintensity=\"0\" envelopespeed=\"0.552000046\" lfowidth=\"0\"/>    <program programname=\"Noisy Mod BP\" cutoff=\"0.428000033\" resonance=\"0.724000037\"             filtertype=\"3\" lfointensity=\"0.716000021\" lforate=\"0.520000041\"             lfosync=\"1\" lfowaveform=\"6\" volume=\"0.936000049\" inputdrive=\"0.780000031\"             envelopeintensity=\"0.752000034\" envelopespeed=\"0.208000004\" lfowidth=\"0\"/>    <program programname=\"SQ Lfo 1/16 LP\" cutoff=\"0.45600003\" resonance=\"0.664000034\"             filtertype=\"1\" lfointensity=\"0.340000004\" lforate=\"0.660000026\"             lfosync=\"2\" lfowaveform=\"4\" volume=\"0.84800005\" inputdrive=\"0.480000019\"             envelopeintensity=\"0.516000032\" envelopespeed=\"0.340000004\" lfowidth=\"1\"/>    <program programname=\"Digital Artefacts HP\" cutoff=\"0.628000021\" resonance=\"0.828000069\"             filtertype=\"2\" lfointensity=\"0.204000011\" lforate=\"0.872000039\"             lfosync=\"1\" lfowaveform=\"5\" volume=\"0.808000028\" inputdrive=\"0.692000031\"             envelopeintensity=\"0.512000024\" envelopespeed=\"0\" lfowidth=\"1\"/>    <program programname=\"Lfo Reso  1/1 BP\" cutoff=\"0.468000025\" resonance=\"0.820000052\"             filtertype=\"3\" lfointensity=\"0.708000004\" lforate=\"0.316000015\"             lfosync=\"6\" lfowaveform=\"3\" volume=\"0.84800005\" inputdrive=\"0.732000053\"             envelopeintensity=\"0.512000024\" envelopespeed=\"0.404000014\" lfowidth=\"0\"/>  </programs>  <midimap/></tal>");
	}

	const String getXmlChunk() 
	{
		return xmlChunk;
	}
	private:
	String xmlChunk;
};
#endif