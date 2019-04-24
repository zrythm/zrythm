/*
	==============================================================================
	This file is part of Tal-Dub-III by Patrick Kunz.

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
		xmlChunk = T("<?xml version=\"1.0\" encoding=\"UTF-8\"?><tal curprogram=\"0\" version=\"1\">  <programs>    <program programname=\"Gentle St Delay\" cutoff=\"0.548000038\" resonance=\"0.432000011\"             inputdrive=\"0.408000022\" delaytime=\"0.239000008\" delaytimesync=\"10\"             delaytwice_l=\"0\" delaytwice_r=\"1\" feedback=\"0.148000002\" highcut=\"0.252000004\"             dry=\"0.5\" wet=\"0.440000027\"/>    <program programname=\"Short St Slap Back\" cutoff=\"0.536000013\" resonance=\"0.484000027\"             inputdrive=\"0.592000008\" delaytime=\"0.272000015\" delaytimesync=\"2\"             delaytwice_l=\"0\" delaytwice_r=\"1\" feedback=\"0\" highcut=\"0.276000023\"             dry=\"0.5\" wet=\"0.364000022\"/>    <program programname=\"Infinite St Delay\" cutoff=\"0.700000048\" resonance=\"0.624000013\"             inputdrive=\"0.340000004\" delaytime=\"0.113000005\" delaytimesync=\"1\"             delaytwice_l=\"0\" delaytwice_r=\"0.800000012\" feedback=\"0.636000037\"             highcut=\"0.124000005\" dry=\"0.5\" wet=\"0.5\"/>    <program programname=\"Fast Delay\" cutoff=\"0.592000008\" resonance=\"0.444000036\"             inputdrive=\"0.300000012\" delaytime=\"0.0510000028\" delaytimesync=\"1\"             delaytwice_l=\"0\" delaytwice_r=\"0\" feedback=\"0.284000009\" highcut=\"0.272000015\"             dry=\"0.5\" wet=\"0.752000034\"/>    <program programname=\"Tab it !\" cutoff=\"0.728000045\" resonance=\"0.724000037\"             inputdrive=\"0.292000026\" delaytime=\"0.102000006\" delaytimesync=\"1\"             delaytwice_l=\"0\" delaytwice_r=\"0\" feedback=\"0.15200001\" highcut=\"0.256000012\"             dry=\"0.5\" wet=\"0.660000026\"/>    <program programname=\"Sweet Drive 1/8\" cutoff=\"0.768000007\" resonance=\"0.492000014\"             inputdrive=\"0.89200002\" delaytime=\"0.41200003\" delaytimesync=\"3\"             delaytwice_l=\"0\" delaytwice_r=\"0\" feedback=\"0.444000036\" highcut=\"0.188000008\"             dry=\"0.5\" wet=\"0.156000003\"/>    <program programname=\"Sweet Drive 1/4.\" cutoff=\"0.672000051\" resonance=\"0.724000037\"             inputdrive=\"0.728000045\" delaytime=\"0.148000002\" delaytimesync=\"10\"             delaytwice_l=\"0\" delaytwice_r=\"0\" feedback=\"0.640000045\" highcut=\"0.340000004\"             dry=\"0.5\" wet=\"0.148000002\"/>    <program programname=\"Thin HP Delay 1/4\" cutoff=\"0.868000031\" resonance=\"0.664000034\"             inputdrive=\"0.824000061\" delaytime=\"0.704000056\" delaytimesync=\"4\"             delaytwice_l=\"0\" delaytwice_r=\"0\" feedback=\"0.460000008\" highcut=\"0.664000034\"             dry=\"0.5\" wet=\"0.24000001\"/>    <program programname=\"Space LP Tab Delay\" cutoff=\"0.296000004\" resonance=\"0.828000069\"             inputdrive=\"0.340000004\" delaytime=\"0.176000014\" delaytimesync=\"1\"             delaytwice_l=\"0\" delaytwice_r=\"0.800000012\" feedback=\"0.336000025\"             highcut=\"0.572000027\" dry=\"0.5\" wet=\"0.736000061\"/>    <program programname=\"Lo Fi Scape\" cutoff=\"0.660000026\" resonance=\"0.820000052\"             inputdrive=\"0.620000005\" delaytime=\"0.0390000008\" delaytimesync=\"1\"             delaytwice_l=\"0\" delaytwice_r=\"0\" feedback=\"0.648000002\" highcut=\"0.263999999\"             dry=\"0.5\" wet=\"0.388000011\"/>  </programs>  <midimap/></tal>");
	}

	const String getXmlChunk() 
	{
		return xmlChunk;
	}
	private:
	String xmlChunk;
};
#endif