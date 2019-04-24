
dofile("../../../scripts/make-project.lua")

package = make_library_project("drowaudio")

package.includepaths = {
  ".",
  "../../juce/source",
  "../../juce/source/modules"
}

package.files = {
  matchfiles (
    "../source/dRowAudio/dRowAudio.cpp"
  )
}
