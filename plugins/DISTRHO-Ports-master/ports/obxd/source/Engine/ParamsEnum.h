/*
	==============================================================================
	This file is part of Obxd synthesizer.

	Copyright © 2013-2014 Filatov Vadim
	
	Contact author via email :
	justdat_@_e1.ru

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
#pragma once
#include "ObxdVoice.h"
enum ObxdParameters
{
	UNDEFINED,
	UNUSED_1,
	VOLUME,
	VOICE_COUNT,
	TUNE,
	OCTAVE,
	BENDRANGE,
	BENDOSC2,
	LEGATOMODE,
	BENDLFORATE,
	VFLTENV,
	VAMPENV,

	ASPLAYEDALLOCATION,
	PORTAMENTO,
	UNISON,		
	UDET,
	OSC2_DET,
	LFOFREQ,
	LFOSINWAVE,LFOSQUAREWAVE,LFOSHWAVE,
	LFO1AMT,LFO2AMT,
	LFOOSC1,LFOOSC2,LFOFILTER,LFOPW1,LFOPW2,
	OSC2HS,
	XMOD,
	OSC1P,
	OSC2P,
	OSCQuantize,
	OSC1Saw,
	OSC1Pul,
	OSC2Saw,
	OSC2Pul,
	PW,
	BRIGHTNESS,
	ENVPITCH,
	OSC1MIX,
	OSC2MIX,
	NOISEMIX,
	FLT_KF,
	CUTOFF,
	RESONANCE,
	MULTIMODE,
	FILTER_WARM,
	BANDPASS,
	FOURPOLE,
	ENVELOPE_AMT,
	LATK,
	LDEC,
	LSUS,
	LREL,
	FATK,
	FDEC,
	FSUS,
	FREL,
	ENVDER,FILTERDER,PORTADER,
	PAN1,PAN2,PAN3,PAN4,PAN5,PAN6,PAN7,PAN8,
	UNUSED_2,
	ECONOMY_MODE,
	LFO_SYNC,
	PW_ENV,
	PW_ENV_BOTH,
	ENV_PITCH_BOTH,
	FENV_INVERT,
	PW_OSC2_OFS,
	LEVEL_DIF,
	SELF_OSC_PUSH,
	PARAM_COUNT,
};
