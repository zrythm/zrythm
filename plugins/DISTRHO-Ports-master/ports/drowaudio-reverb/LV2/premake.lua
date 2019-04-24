
dofile("../../../scripts/make-project.lua")

package = make_juce_lv2_project("drowaudio-reverb")

package.config["Release"].links = { package.config["Release"].links, "drowaudio" }
package.config["Debug"].links   = { package.config["Debug"].links, "drowaudio_debug" }

package.defines = { package.defines, "USE_DROWAUDIO=1" }

package.includepaths = {
  package.includepaths,
  "../../drowaudio-common"
}

if (os.getenv("LINUX_EMBED")) then
package.files = {
  matchfiles (
    "../source/DRowAudioFilter.cpp",
    "../../drowaudio-common/dRowAudio_AllpassFilter.cpp",
    "../../drowaudio-common/dRowAudio_DelayRegister.cpp",
    "../../drowaudio-common/dRowAudio_LBCF.cpp",
    "../../drowaudio-common/dRowAudio_TappedDelayLine.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
else
package.files = {
  matchfiles (
    "../source/*.cpp",
    "../../drowaudio-common/dRowAudio_AllpassFilter.cpp",
    "../../drowaudio-common/dRowAudio_DelayRegister.cpp",
    "../../drowaudio-common/dRowAudio_LBCF.cpp",
    "../../drowaudio-common/dRowAudio_PluginLookAndFeel.cpp",
    "../../drowaudio-common/dRowAudio_TappedDelayLine.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
end
