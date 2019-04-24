
dofile("../../../scripts/make-project.lua")

package = make_juce_lv2_project("TAL-Vocoder-2")

package.includepaths = {
  package.includepaths,
  "../source/engine",
  "../source/engine/synth",
  "." --fake
}

if (os.getenv("LINUX_EMBED")) then
package.files = {
  matchfiles (
    "../source/TalCore.cpp",
    "../source/engine/vocoder/*.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
else
package.files = {
  matchfiles (
    "../source/*.cpp",
    "../source/engine/vocoder/*.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
end
