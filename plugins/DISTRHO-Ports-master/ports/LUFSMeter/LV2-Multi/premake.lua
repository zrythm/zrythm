
dofile("../../../scripts/make-project.lua")

package = make_juce_lv2_project("LUFSMeterMulti")

package.defines = { package.defines, "LUFS_MULTI=1" }

package.files = {
  matchfiles (
    "../source/*.cpp",
    "../source/filters/*.cpp",
    "../source/gui/*.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
