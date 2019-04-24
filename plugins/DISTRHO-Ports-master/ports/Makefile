# -----------------------------------------
# Compile all the Ports

all: libs lv2 vst

plugins: lv2 vst

# -----------------------------------------
# libs

libs:
	$(MAKE) -C ../libs/drowaudio
	$(MAKE) -C ../libs/juce
	$(MAKE) -C ../libs/juced
	$(MAKE) -C ../libs/lv2-ttl-generator

# -----------------------------------------
# lv2

lv2: libs
	install -d ../bin/lv2
ifeq ($(LINUX_EMBED),true)
	$(MAKE) -C arctican-function/LV2
	$(MAKE) -C arctican-pilgrim/LV2
	$(MAKE) -C drowaudio-distortion/LV2
	$(MAKE) -C drowaudio-distortionshaper/LV2
	$(MAKE) -C drowaudio-flanger/LV2
	$(MAKE) -C drowaudio-reverb/LV2
	$(MAKE) -C drowaudio-tremolo/LV2
	$(MAKE) -C luftikus/LV2
	$(MAKE) -C obxd/LV2
	$(MAKE) -C tal-dub-3/LV2
	$(MAKE) -C tal-filter/LV2
	$(MAKE) -C tal-filter-2/LV2
	$(MAKE) -C tal-noisemaker/LV2
	$(MAKE) -C tal-reverb/LV2
	$(MAKE) -C tal-reverb-2/LV2
	$(MAKE) -C tal-reverb-3/LV2
	$(MAKE) -C tal-vocoder-2/LV2
	$(MAKE) -C vex/LV2
	$(MAKE) -C wolpertinger/LV2
else
	$(MAKE) -C arctican-function/LV2
	$(MAKE) -C arctican-pilgrim/LV2
	$(MAKE) -C dexed/LV2
	$(MAKE) -C drowaudio-distortion/LV2
	$(MAKE) -C drowaudio-distortionshaper/LV2
	$(MAKE) -C drowaudio-flanger/LV2
	$(MAKE) -C drowaudio-reverb/LV2
	$(MAKE) -C drowaudio-tremolo/LV2
	$(MAKE) -C drumsynth/LV2
	$(MAKE) -C easySSP/LV2
	$(MAKE) -C eqinox/LV2
	$(MAKE) -C HiReSam/LV2
# 	$(MAKE) -C juce-demo-host/LV2
	$(MAKE) -C juce-demo-plugin/LV2
	$(MAKE) -C juce-opl/LV2
	$(MAKE) -C klangfalter/LV2
	$(MAKE) -C LUFSMeter/LV2
	$(MAKE) -C LUFSMeter/LV2-Multi
	$(MAKE) -C luftikus/LV2
	$(MAKE) -C obxd/LV2
	$(MAKE) -C pitchedDelay/LV2
	$(MAKE) -C refine/LV2
	$(MAKE) -C stereosourceseparation/LV2
	$(MAKE) -C tal-dub-3/LV2
	$(MAKE) -C tal-filter/LV2
	$(MAKE) -C tal-filter-2/LV2
	$(MAKE) -C tal-noisemaker/LV2
	$(MAKE) -C tal-reverb/LV2
	$(MAKE) -C tal-reverb-2/LV2
	$(MAKE) -C tal-reverb-3/LV2
	$(MAKE) -C tal-vocoder-2/LV2
	$(MAKE) -C vex/LV2
	$(MAKE) -C wolpertinger/LV2
endif

# -----------------------------------------
# vst

vst: libs
	install -d ../bin/vst
	$(MAKE) -C arctican-function/VST
	$(MAKE) -C arctican-pilgrim/VST
	$(MAKE) -C dexed/VST
	$(MAKE) -C drowaudio-distortion/VST
	$(MAKE) -C drowaudio-distortionshaper/VST
	$(MAKE) -C drowaudio-flanger/VST
	$(MAKE) -C drowaudio-reverb/VST
	$(MAKE) -C drowaudio-tremolo/VST
	$(MAKE) -C drumsynth/VST
	$(MAKE) -C easySSP/VST
	$(MAKE) -C eqinox/VST
	$(MAKE) -C HiReSam/VST
# 	$(MAKE) -C juce-demo-host/VST
	$(MAKE) -C juce-demo-plugin/VST
	$(MAKE) -C juce-opl/VST
	$(MAKE) -C klangfalter/VST
	$(MAKE) -C LUFSMeter/VST
	$(MAKE) -C LUFSMeter/VST-Multi
	$(MAKE) -C luftikus/VST
	$(MAKE) -C obxd/VST
	$(MAKE) -C pitchedDelay/VST
	$(MAKE) -C refine/VST
	$(MAKE) -C stereosourceseparation/VST
	$(MAKE) -C tal-dub-3/VST
	$(MAKE) -C tal-filter/VST
	$(MAKE) -C tal-filter-2/VST
	$(MAKE) -C tal-noisemaker/VST
	$(MAKE) -C tal-reverb/VST
	$(MAKE) -C tal-reverb-2/VST
	$(MAKE) -C tal-reverb-3/VST
	$(MAKE) -C tal-vocoder-2/VST
	$(MAKE) -C vex/VST
	$(MAKE) -C wolpertinger/VST

# -----------------------------------------
# clean

clean:
	# LV2
	$(MAKE) clean -C arctican-function/LV2
	$(MAKE) clean -C arctican-pilgrim/LV2
	$(MAKE) clean -C dexed/LV2
	$(MAKE) clean -C drowaudio-distortion/LV2
	$(MAKE) clean -C drowaudio-distortionshaper/LV2
	$(MAKE) clean -C drowaudio-flanger/LV2
	$(MAKE) clean -C drowaudio-reverb/LV2
	$(MAKE) clean -C drowaudio-tremolo/LV2
	$(MAKE) clean -C drumsynth/LV2
	$(MAKE) clean -C easySSP/LV2
	$(MAKE) clean -C eqinox/LV2
	$(MAKE) clean -C HiReSam/LV2
	$(MAKE) clean -C juce-demo-host/LV2
	$(MAKE) clean -C juce-demo-plugin/LV2
	$(MAKE) clean -C juce-opl/LV2
	$(MAKE) clean -C klangfalter/LV2
	$(MAKE) clean -C LUFSMeter/LV2
	$(MAKE) clean -C LUFSMeter/LV2-Multi
	$(MAKE) clean -C luftikus/LV2
	$(MAKE) clean -C obxd/LV2
	$(MAKE) clean -C pitchedDelay/LV2
	$(MAKE) clean -C refine/LV2
	$(MAKE) clean -C stereosourceseparation/LV2
	$(MAKE) clean -C tal-dub-3/LV2
	$(MAKE) clean -C tal-filter/LV2
	$(MAKE) clean -C tal-filter-2/LV2
	$(MAKE) clean -C tal-noisemaker/LV2
	$(MAKE) clean -C tal-reverb/LV2
	$(MAKE) clean -C tal-reverb-2/LV2
	$(MAKE) clean -C tal-reverb-3/LV2
	$(MAKE) clean -C tal-vocoder-2/LV2
	$(MAKE) clean -C vex/LV2
	$(MAKE) clean -C wolpertinger/LV2

	# VST
	$(MAKE) clean -C arctican-function/VST
	$(MAKE) clean -C arctican-pilgrim/VST
	$(MAKE) clean -C dexed/VST
	$(MAKE) clean -C drowaudio-distortion/VST
	$(MAKE) clean -C drowaudio-distortionshaper/VST
	$(MAKE) clean -C drowaudio-flanger/VST
	$(MAKE) clean -C drowaudio-reverb/VST
	$(MAKE) clean -C drowaudio-tremolo/VST
	$(MAKE) clean -C drumsynth/VST
	$(MAKE) clean -C easySSP/VST
	$(MAKE) clean -C eqinox/VST
	$(MAKE) clean -C HiReSam/LV2
	$(MAKE) clean -C juce-demo-host/VST
	$(MAKE) clean -C juce-demo-plugin/VST
	$(MAKE) clean -C juce-opl/VST
	$(MAKE) clean -C klangfalter/VST
	$(MAKE) clean -C LUFSMeter/VST
	$(MAKE) clean -C LUFSMeter/VST-Multi
	$(MAKE) clean -C luftikus/VST
	$(MAKE) clean -C obxd/VST
	$(MAKE) clean -C pitchedDelay/VST
	$(MAKE) clean -C refine/VST
	$(MAKE) clean -C stereosourceseparation/VST
	$(MAKE) clean -C tal-dub-3/VST
	$(MAKE) clean -C tal-filter/VST
	$(MAKE) clean -C tal-filter-2/VST
	$(MAKE) clean -C tal-noisemaker/VST
	$(MAKE) clean -C tal-reverb/VST
	$(MAKE) clean -C tal-reverb-2/VST
	$(MAKE) clean -C tal-reverb-3/VST
	$(MAKE) clean -C tal-vocoder-2/VST
	$(MAKE) clean -C vex/VST
	$(MAKE) clean -C wolpertinger/VST

	rm -rf */LV2/intermediate
	rm -rf */VST/intermediate

distclean: clean
	rm -rf */LV2/*.lv2
	rm -f */*/Makefile
	rm -f */*/*.make
