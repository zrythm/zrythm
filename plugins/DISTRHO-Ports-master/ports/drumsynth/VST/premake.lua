
dofile("../../../scripts/make-project.lua")

package = make_juce_vst_project("drumsynth")

package.config["Release"].links = { package.config["Release"].links, "juced" }
package.config["Debug"].links   = { package.config["Debug"].links, "juced_debug" }

package.defines = { package.defines, "USE_JUCED=1" }

package.files = {
  matchfiles (
    "../source/*.cpp",
    "../source/Components/*.cpp",
    "../source/DrumSynth/*.cpp",
    "../source/IniParser/*.cpp",
    "../source/Resources/*.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
