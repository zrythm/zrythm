
dofile("../../../scripts/make-project.lua")

package = make_juce_lv2_project("TAL-Filter")

package.includepaths = {
  package.includepaths,
  "../source/Engine",
  "." --fake
}

if (os.getenv("LINUX_EMBED")) then
package.files = {
  matchfiles (
    "../source/TalCore.cpp",
    "../source/Engine/*.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
else
package.files = {
  matchfiles (
    "../source/*.cpp",
    "../source/Engine/*.cpp",
    "../../../libs/juce-plugin/JucePluginMain.cpp"
  )
}
end
