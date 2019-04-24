
dofile("../../../scripts/make-project.lua")

package = make_library_project("juced")

package.includepaths = {
  ".",
  "../../juce/source",
  "../../juce/source/modules"
}

package.files = {
  matchfiles (
    "../source/juced.cpp"
  )
}
