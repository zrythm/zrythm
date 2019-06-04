## Release Notes

- Switched from autotools to meson
- Fix all compiler warnings
- Fix snapping miscalculation where an off-grid selection would not snap to the grid but move by 1 snap point instead
- Fix some memory leaks
- Implement new system for showing arranger objects where each object holds pointers to its transients and lane counterparts
- Each track can now have multiple lanes
- Remove DISTRHO ports from tree
- Add additional pinned timeline above the main one
- Show icon of track inside its color box instead of next to it
- Add option to generate coverage reports
- Remove google fonts from docs
- Code cleanup
- Add Control Room functionality (WIP)
- Add Contributing Guidelines to the docs
- Add Modulator tab for automating plugin controls (WIP)
- Implement backward/forward button functionality
