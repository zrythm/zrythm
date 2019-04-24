
dofile("../../../scripts/make-project.lua")

package = make_juce_vst_project("TAL-Filter")

package.includepaths = {
  package.includepaths,
  "../source/Engine",
  "." --fake
}

package.files = {
  matchfiles (
    "../source/*.cpp",
    "../source/Engine/*.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
