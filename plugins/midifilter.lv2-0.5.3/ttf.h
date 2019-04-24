#ifndef _TTF_H_
#define _TTF_H_

/* cfg-port offsets 0,1, are midi ports, . */
#define ADDP_0 3
#define ADDP_1 4
#define ADDP_2 5
#define ADDP_3 6
#define ADDP_4 7
#define ADDP_5 8
#define ADDP_6 9
#define ADDP_7 10
#define ADDP_8 11
#define ADDP_9 12
#define ADDP_10 13
#define ADDP_11 14
#define ADDP_12 15
#define ADDP_13 16
#define ADDP_14 17
#define ADDP_15 18

#define PORTIDX(x) ADDP_##x

#define MAINTAINER <HTTPP/gareus.org/rgareus#me>
#define MIDIEXTURI <HTTPP/lv2plug.in/ns/ext/midi#MidiEvent>

#define TTF_DEF(DOAPNAME, MODNAME, INSUPPORTS) \
	a lv2:Plugin, lv2:MIDIPlugin ; \
	doap:name DOAPNAME ; \
	mod:label MODNAME ; \
	@MODBRAND@ \
	@VERSION@ \
	doap:license <HTTPP/usefulinc.com/doap/licenses/gpl> ; \
  lv2:project <HTTPP/gareus.org/oss/lv2/midifilter> ; \
	lv2:optionalFeature lv2:hardRTCapable ; \
	lv2:requiredFeature urid:map ; \
	lv2:port \
	[ \
		a atom:AtomPort , \
			lv2:InputPort ; \
		atom:bufferType atom:Sequence ; \
		atom:supports MIDIEXTURI INSUPPORTS; \
		lv2:index 0 ; \
		lv2:symbol "midiin" ; \
		lv2:name "MIDI In" \
	] , [ \
		a atom:AtomPort , \
			lv2:OutputPort ; \
		atom:bufferType atom:Sequence ; \
		atom:supports MIDIEXTURI ; \
		lv2:index 1 ; \
		lv2:symbol "midiout" ; \
		lv2:name "MIDI Out"; \
	] , [ \
		a lv2:OutputPort, \
			lv2:ControlPort ; \
		lv2:name "latency" ; \
		lv2:index 2 ; \
		lv2:symbol "latency" ; \
		lv2:minimum 0 ; \
		lv2:maximum 192000 ; \
		lv2:portProperty lv2:reportsLatency, lv2:integer, pprops:notOnGUI; \
	]

#define TTF_DEFAULTDEF(DOAPNAME, MODNAME) TTF_DEF(DOAPNAME, MODNAME,)

#define TTF_PORT(TYPE, IDX, SYM, DESC, VMIN, VMAX, VDFLT, ATTR) \
	[ \
    a TYPE, \
      lv2:ControlPort ; \
    lv2:index PORTIDX(IDX) ; \
    lv2:symbol SYM ; \
    lv2:name DESC; \
    lv2:minimum VMIN ; \
    lv2:maximum VMAX ; \
    lv2:default VDFLT; \
    ATTR \
  ]

#define TTF_IPORT(IDX, SYM, DESC, VMIN, VMAX, VDFLT, ATTR) \
	TTF_PORT(lv2:InputPort, IDX, SYM, DESC, VMIN, VMAX, VDFLT, ATTR)

#define TTF_IPORTFLOAT(IDX, SYM, DESC, VMIN, VMAX, VDFLT) \
	TTF_IPORT(IDX, SYM, DESC, VMIN, VMAX, VDFLT, )

#define TTF_IPORTINT(IDX, SYM, DESC, VMIN, VMAX, VDFLT) \
	TTF_IPORT(IDX, SYM, DESC, VMIN, VMAX, VDFLT, lv2:portProperty lv2:integer)

#define TTF_IPORTTOGGLE(IDX, SYM, DESC, VDFLT) \
	TTF_IPORT(IDX, SYM, DESC, 0, 1, VDFLT, lv2:portProperty lv2:integer; lv2:portProperty lv2:toggled)

#define PORTENUM16 \
	lv2:scalePoint [ rdfs:label "01" ; rdf:value  1 ] ; \
	lv2:scalePoint [ rdfs:label "02" ; rdf:value  2 ] ; \
	lv2:scalePoint [ rdfs:label "03" ; rdf:value  3 ] ; \
	lv2:scalePoint [ rdfs:label "04" ; rdf:value  4 ] ; \
	lv2:scalePoint [ rdfs:label "05" ; rdf:value  5 ] ; \
	lv2:scalePoint [ rdfs:label "06" ; rdf:value  6 ] ; \
	lv2:scalePoint [ rdfs:label "07" ; rdf:value  7 ] ; \
	lv2:scalePoint [ rdfs:label "08" ; rdf:value  8 ] ; \
	lv2:scalePoint [ rdfs:label "09" ; rdf:value  9 ] ; \
	lv2:scalePoint [ rdfs:label "10" ; rdf:value 10 ] ; \
	lv2:scalePoint [ rdfs:label "11" ; rdf:value 11 ] ; \
	lv2:scalePoint [ rdfs:label "12" ; rdf:value 12 ] ; \
	lv2:scalePoint [ rdfs:label "13" ; rdf:value 13 ] ; \
	lv2:scalePoint [ rdfs:label "14" ; rdf:value 14 ] ; \
	lv2:scalePoint [ rdfs:label "15" ; rdf:value 15 ] ; \
	lv2:scalePoint [ rdfs:label "16" ; rdf:value 16 ] ; \
	lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration;

#define SPX(LBL,VAL) \
	lv2:scalePoint [ rdfs:label LBL ; rdf:value VAL ] ;

#define PORTENUMZ(ZEROLABEL) \
	lv2:scalePoint [ rdfs:label ZEROLABEL; rdf:value  0 ] ; \
	PORTENUM16

#define DOC_CHANZ \
	rdfs:comment "MIDI Channel number 1..16 (0: disable channel)"

#define DOC_CHANF \
	rdfs:comment "MIDI Channel (1..16) on which the filter is active; 0: any). Data on other channels is passed though unmodified."

#define NOTENAMES \
	lv2:scalePoint [ rdfs:label "C"  ; rdf:value 0 ] ; \
	lv2:scalePoint [ rdfs:label "C#" ; rdf:value 1 ] ; \
	lv2:scalePoint [ rdfs:label "D"  ; rdf:value 2 ] ; \
	lv2:scalePoint [ rdfs:label "D#" ; rdf:value 3 ] ; \
	lv2:scalePoint [ rdfs:label "E"  ; rdf:value 4 ] ; \
	lv2:scalePoint [ rdfs:label "F"  ; rdf:value 5 ] ; \
	lv2:scalePoint [ rdfs:label "F#" ; rdf:value 6 ] ; \
	lv2:scalePoint [ rdfs:label "G"  ; rdf:value 7 ] ; \
	lv2:scalePoint [ rdfs:label "G#" ; rdf:value 8 ] ; \
	lv2:scalePoint [ rdfs:label "A"  ; rdf:value 9 ] ; \
	lv2:scalePoint [ rdfs:label "A#" ; rdf:value 10 ] ; \
	lv2:scalePoint [ rdfs:label "B"  ; rdf:value 11 ] ; \
	lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration;

#define NOTENAMESOFF \
	lv2:scalePoint [ rdfs:label "OFF" ; rdf:value -1 ] ; \
	NOTENAMES

#endif

/* variable part */

#ifdef MFD_FLT
#undef MFD_FLT
#endif

#ifdef MX_FILTER
#define MFD_FLT(ID, FNX) \
	else if (!strcmp(descriptor->URI, MFP_URI "#" # FNX)) { self->filter_fn = filter_midi_ ## FNX; filter_init_ ## FNX(self); }

#elif (defined MX_DESC)

#define MFD_FLT(ID, FNX) \
	MF_DESCRIPTOR(ID, "" # FNX)

#elif (defined MX_MANIFEST)

#define MFD_FLT(ID, FNX) \
  <HTTPP/gareus.org/oss/lv2/midifilterHASH ## FNX> \
	a lv2:Plugin ; \
	lv2:binary <@LV2NAME@@LIB_EXT@>  ; \
	rdfs:seeAlso <@LV2NAME@.ttl> . \

#elif (defined MX_MODGUI)

#define MFD_FLT(ID, FNX) \
	<HTTPP/gareus.org/oss/lv2/midifilterHASH ## FNX> \
	modgui:gui [ \
		modgui:resourcesDirectory <modgui> ; \
		modgui:iconTemplate <modgui/icon_DASH_ ## FNX ##_DOT_html> ; \
		modgui:stylesheet   <modgui/x42-style.css> ; \
		modgui:screenshot   <modgui/screenshot_DASH_ ## FNX ##_DOT_png> ; \
		modgui:thumbnail    <modgui/thumbnail_DASH_ ## FNX ##_DOT_png> ; \
		modgui:brand "x42" ; \
		modgui:label # FNX ; \
	].

#else

#define MFD_FLT(ID, FNX)

#endif
